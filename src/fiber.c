#ifndef CONFIG_MMAP_ALLOW_UNINITIALIZED
# define CONFIG_MMAP_ALLOW_UNINITIALIZED
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "private.h"

melon_fiber * melon_fiber_self(void)
{
  return g_current_fiber;
}

void melon_fiber_destroy(melon_fiber * fiber)
{
  melon_stack_free(fiber->ctx.uc_stack.ss_sp);
  pthread_spin_destroy(&fiber->lock);
  free(fiber);
}

static void melon_fiber_wrapper(void)
{
  melon_fiber * self = melon_fiber_self();
  int destroy = 0;
  assert(self);
  assert(!self->ctx.uc_link);
  assert(self->callback);

  self->callback(self->callback_ctx);

  /* now join or destroy */
  pthread_spin_lock(&self->lock);
  if (self->is_detached)
  {
    /* push in the destroy queue */
    self->waited_event = kEventDestroy;
    destroy = 1;
    pthread_spin_unlock(&self->lock);
  }
  else
  {
    self->waited_event = kEventJoin;
    if (self->terminate_waiter)
    {
      pthread_spin_unlock(&self->lock);
      /* a fiber is waiting for us, let's activate it ! */
      self->terminate_waiter->waited_event = kEventNone;
      melon_sched_ready(self->terminate_waiter);
      destroy = 1;
    }
    else
      pthread_spin_unlock(&self->lock);
  }

  /* decrement the fibers count and if 0, then broadcast the cond */
  if (__sync_add_and_fetch(&g_melon.fibers_count, -1) == 0)
    pthread_cond_broadcast(&g_melon.fibers_count_zero);
  melon_sched_next(destroy);
  assert(0 && "should never be reached");
}

melon_fiber * melon_fiber_start(void (*fct)(void *), void * ctx)
{
  assert(fct);

  /* allocate a fiber structre */
  melon_fiber * fiber = malloc(sizeof (melon_fiber));
  if (!fiber)
    return NULL;
  fiber->next             = NULL;
  fiber->timeout          = 0;
  memset(&fiber->ctx, 0, sizeof (fiber->ctx));
  fiber->waited_event     = kEventNone;
  fiber->is_detached      = 0;
  fiber->terminate_waiter = NULL;
  fiber->name             = "(none)";
  fiber->callback         = fct;
  fiber->callback_ctx     = ctx;
  if (pthread_spin_init(&fiber->lock, PTHREAD_PROCESS_SHARED))
  {
    free(fiber);
    return NULL;
  }

  /* allocate the stack, TODO: have a stack allocator with a cache */
  int ret = getcontext(&fiber->ctx);
  assert(!ret);
  fiber->ctx.uc_link = NULL;
  fiber->ctx.uc_stack.ss_size = melon_stack_size();
  fiber->ctx.uc_stack.ss_sp = melon_stack_alloc();
  if (!fiber->ctx.uc_stack.ss_sp)
  {
    free(fiber);
    return NULL;
  }
  makecontext(&fiber->ctx, melon_fiber_wrapper, 0);
  __sync_fetch_and_add(&g_melon.fibers_count, 1);
  melon_sched_ready(fiber);
  return fiber;
}

void melon_fiber_join(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.lock);
  if (fiber->waited_event == kEventJoin)
  {
    pthread_mutex_unlock(&g_melon.lock);
    melon_fiber_destroy(fiber);
  }
  else
  {
    /* check that the user doesn't join the fiber from two differents fibers */
    if (fiber->terminate_waiter)
    {
      fprintf(stderr, "panic: it's incorrect to join the same fiber from two differents fibers\n");
      fflush(stderr);
      abort();
    }

    if (fiber->is_detached)
    {
      fprintf(stderr, "panic: it's incorrect to join a detached fiber\n");
      fflush(stderr);
      abort();
    }

    melon_fiber * self = melon_fiber_self();
    fiber->terminate_waiter = self;
    /* now let's wait for the fiber to finish */
    melon_sched_next(0);
    /* here the fiber has been waked up so the fiber is finished !
     * We have the responsability to free the fiber. */
    melon_fiber_destroy(fiber);
  }
}

int melon_fiber_tryjoin(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.lock);
  if (fiber->waited_event == kEventJoin)
  {
    pthread_mutex_unlock(&g_melon.lock);
    melon_fiber_destroy(fiber);
    return 1;
  }
  pthread_mutex_unlock(&g_melon.lock);
  return 0;
}

void melon_fiber_detach(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.lock);
  if (fiber->waited_event == kEventJoin)
    melon_fiber_destroy(fiber);
  else
    fiber->is_detached = 1;
  pthread_mutex_unlock(&g_melon.lock);
}
