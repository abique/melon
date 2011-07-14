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
  free(fiber);
}

static void melon_fiber_wrapper(void)
{
  melon_fiber * self    = melon_fiber_self();

  assert(self);
  assert(!self->ctx.uc_link);
  assert(self->callback);
  self->callback(self->callback_ctx);
  self->waited_event = kEventDestroy;

  /* decrement the fibers count and if 0, then broadcast the cond */
  if (__sync_add_and_fetch(&g_melon.fibers_count, -1) == 0)
    pthread_cond_broadcast(&g_melon.fibers_count_zero);

  melon_sched_next();
  assert(0 && "should never be reached");
}

int melon_fiber_start(void (*fct)(void *), void * ctx)
{
  assert(fct);

  /* allocate a fiber structre */
  melon_fiber * fiber = malloc(sizeof (melon_fiber));
  if (!fiber)
    return -1;
  fiber->next             = NULL;
  fiber->timer            = 0;
  fiber->timer_next       = NULL;
  memset(&fiber->ctx, 0, sizeof (fiber->ctx));
  fiber->waited_event     = kEventNone;
  fiber->is_detached      = 0;
  fiber->name             = "(none)";
  fiber->callback         = fct;
  fiber->callback_ctx     = ctx;

  /* allocate the stack, TODO: have a stack allocator with a cache */
  int ret = getcontext(&fiber->ctx);
  assert(!ret);
  fiber->ctx.uc_link = NULL;
  fiber->ctx.uc_stack.ss_size = melon_stack_size();
  fiber->ctx.uc_stack.ss_sp = melon_stack_alloc();
  if (!fiber->ctx.uc_stack.ss_sp)
  {
    free(fiber);
    return -1;
  }
  makecontext(&fiber->ctx, melon_fiber_wrapper, 0);
  __sync_fetch_and_add(&g_melon.fibers_count, 1);
  melon_sched_ready(fiber);
  return 0;
}
