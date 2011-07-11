#ifndef CONFIG_MMAP_ALLOW_UNINITIALIZED
# define CONFIG_MMAP_ALLOW_UNINITIALIZED
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "private.h"

#ifndef MAP_UNINITIALIZED
# define MAP_UNINITIALIZED 0x0
#endif

melon_fiber * melon_fiber_self(void)
{
  return g_current_fiber;
}

void melon_fiber_destroy(melon_fiber * fiber)
{
  munmap(fiber->ctx.uc_stack.ss_sp, fiber->ctx.uc_stack.ss_size);
  free(fiber);
}

struct callback
{
  void (*fct)(void *);
  void * ctx;
};

static void melon_fiber_wrapper(struct callback cb)
{
  assert(g_current_fiber);
  assert(!g_current_fiber->ctx.uc_link);
  cb.fct(cb.ctx);

  /* now join or destroy */
  melon_fiber * self = melon_fiber_self();
  pthread_mutex_lock(&g_melon.mutex);
  if (self->is_detached)
  {
    /* push in the destroy queue */
    self->waited_event = kEventDestroy;
    melon_list_push(g_melon.destroy, self);
  }
  else
  {
    self->waited_event = kEventJoin;
    if (self->terminate_waiter)
    {
      /* a fiber is waiting for us, let's activate it ! */
      self->terminate_waiter->waited_event = kEventNone;
      melon_list_push(g_melon.ready, self->terminate_waiter);
    }
  }

  /* decrement the fibers count and if 0, then broadcast the cond */
  if (--g_melon.fibers_count == 0)
    pthread_cond_broadcast(&g_melon.fibers_count_zero);
  pthread_mutex_unlock(&g_melon.mutex);
  melon_sched_next();
  assert(0 && "should never be reached");
}

melon_fiber * melon_fiber_start(void (*fct)(void *), void * ctx)
{
  static int PAGE_SIZE = 0;

  if (!PAGE_SIZE)
    PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

  assert(fct);

  /* allocate a fiber structre */
  melon_fiber * fiber = calloc(1, sizeof (melon_fiber));
  if (!fiber)
    return NULL;
  fiber->waited_event = kEventNone;

  /* allocate the stack, TODO: have a stack allocator with a cache */
  int ret = getcontext(&fiber->ctx);
  assert(!ret);
  fiber->ctx.uc_link = NULL;
  fiber->ctx.uc_stack.ss_size = PAGE_SIZE;
  fiber->ctx.uc_stack.ss_sp = mmap(0, PAGE_SIZE * 2, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED | MAP_GROWSDOWN | MAP_STACK,
                                   0, 0) + PAGE_SIZE;
  if (!fiber->ctx.uc_stack.ss_sp)
  {
    free(fiber);
    return NULL;
  }

  /* initialize the context */
  struct callback cb;
  cb.fct = fct;
  cb.ctx = ctx;
  makecontext(&fiber->ctx, (void (*)(void))melon_fiber_wrapper,
              sizeof (cb) / sizeof (int), cb);

  pthread_mutex_lock(&g_melon.mutex);
  ++g_melon.fibers_count;
  melon_list_push(g_melon.ready, fiber);
  pthread_cond_signal(&g_melon.ready_cond);
  pthread_mutex_unlock(&g_melon.mutex);
  return fiber;
}

void melon_fiber_join(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.mutex);
  if (fiber->waited_event == kEventJoin)
  {
    pthread_mutex_unlock(&g_melon.mutex);
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
    melon_sched_next();
    /* here the fiber has been waked up so the fiber is finished !
     * We have the responsability to free the fiber. */
    melon_fiber_destroy(fiber);
  }
}

int melon_fiber_tryjoin(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.mutex);
  if (fiber->waited_event == kEventJoin)
  {
    pthread_mutex_unlock(&g_melon.mutex);
    melon_fiber_destroy(fiber);
    return 1;
  }
  pthread_mutex_unlock(&g_melon.mutex);
  return 0;
}

void melon_fiber_detach(melon_fiber * fiber)
{
  assert(fiber);
  pthread_mutex_lock(&g_melon.mutex);
  if (fiber->waited_event == kEventJoin)
    melon_fiber_destroy(fiber);
  else
    fiber->is_detached = 1;
  pthread_mutex_unlock(&g_melon.mutex);
}
