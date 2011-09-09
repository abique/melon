#ifndef CONFIG_MMAP_ALLOW_UNINITIALIZED
# define CONFIG_MMAP_ALLOW_UNINITIALIZED
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "private.h"

int melon_fiberattr_init(melon_fiberattr ** attr)
{
  *attr = malloc(sizeof (**attr));
  if (!*attr)
    return -1;
  (*attr)->name = "(none)";
  (*attr)->stack_size = melon_stack_size();
  return 0;
}

void melon_fiberattr_destroy(melon_fiberattr * attr)
{
  free(attr);
}

void melon_fiberattr_setstacksize(melon_fiberattr * attr, uint32_t size)
{
  attr->stack_size = size;
}

int melon_fiberattr_getstacksize(melon_fiberattr * attr)
{
  return attr->stack_size;
}

melon_fiber * melon_fiber_self(void)
{
  return g_current_fiber;
}

void melon_fiber_destroy(melon_fiber * fiber)
{
  melon_stack_free(fiber->ctx.uc_stack.ss_sp);
  if (fiber->join_mutex)
  {
    melon_cond_destroy(fiber->join_cond);
    melon_mutex_destroy(fiber->join_mutex);
  }

  free(fiber);
  if (g_current_fiber == fiber)
    g_current_fiber = NULL;
}

static void melon_fiber_wrapper(void)
{
  melon_fiber * self    = melon_fiber_self();

  assert(self);
  assert(!self->ctx.uc_link);
  assert(self->start_cb);

  self->join_retval = self->start_cb(self->start_ctx);
  if (self->join_mutex)
  {
    melon_mutex_lock(self->join_mutex);
    self->join_is_finished = 1;
    melon_cond_broadcast(self->join_cond);
    if (!self->join_is_detached)
      melon_cond_wait(self->join_cond, self->join_mutex);
    melon_mutex_unlock(self->join_mutex);
  }

  self->waited_event = kEventDestroy;

  /* decrement the fibers count and if 0, then broadcast the cond */
  if (__sync_add_and_fetch(&g_melon.fibers_count, -1) == 0)
    pthread_cond_broadcast(&g_melon.fibers_count_zero);

  self->sched_next_cb  = (void(*)(void*))melon_fiber_destroy;
  self->sched_next_ctx = self;
  melon_sched_next();
  assert(0 && "should never be reached");
}

static melon_fiber * __melon_fiber_start(melon_fiberattr * attr,
                                         void *            (*fct)(void *),
                                         void *            ctx,
                                         int               joinable)
{
  assert(fct);

  /* allocate a fiber structre */
  melon_fiber * fiber = malloc(sizeof (melon_fiber));
  if (!fiber)
    return NULL;
  fiber->next             = NULL;
  fiber->prev             = NULL;
  fiber->timer            = 0;
  fiber->timer_next       = NULL;
  fiber->timer_prev       = NULL;
  fiber->waited_event     = kEventNone;
  fiber->name             = attr ? attr->name : "(none)";
  fiber->io_canceled      = 0;
  fiber->start_cb         = fct;
  fiber->start_ctx        = ctx;
  fiber->sched_next_cb    = NULL;
  fiber->sched_next_ctx   = NULL;
  fiber->cond_mutex       = NULL;
  fiber->lock_count       = 0;
  fiber->sem_nb           = 0;
  fiber->join_retval      = NULL;
  fiber->join_is_detached = 0;
  fiber->join_is_finished = 0;
  fiber->join_mutex       = NULL;
  fiber->join_cond        = NULL;
  melon_spin_init(&fiber->lock);
  if (joinable)
  {
    if (melon_mutex_init(&fiber->join_mutex, NULL))
      goto failure_mutex;

    if (melon_cond_init(&fiber->join_cond, NULL))
      goto failure_cond;
  }

  /* allocate the stack, TODO: have a stack allocator with a cache */
  int ret = getcontext(&fiber->ctx);
  assert(!ret);
  fiber->ctx.uc_link = NULL;
  fiber->ctx.uc_stack.ss_size = melon_stack_size();
  fiber->ctx.uc_stack.ss_sp = melon_stack_alloc();
  if (!fiber->ctx.uc_stack.ss_sp)
    goto failure_stack;
  makecontext(&fiber->ctx, melon_fiber_wrapper, 0);
  __sync_fetch_and_add(&g_melon.fibers_count, 1);
  melon_sched_ready(fiber);
  return fiber;

  failure_stack:
  if (fiber->join_cond)
    melon_cond_destroy(fiber->join_cond);
  failure_cond:
  if (fiber->join_mutex)
    melon_mutex_destroy(fiber->join_mutex);
  failure_mutex:
  free(fiber);
  return NULL;
}

int melon_fiber_createlight(melon_fiberattr * attr, void * (*fct)(void *), void * ctx)
{
  return __melon_fiber_start(attr, fct, ctx, 0) ? 0 : -1;
}

int melon_fiber_create(melon_fiber ** fiber, melon_fiberattr * attr, void * (*fct)(void *), void * ctx)
{
  *fiber = __melon_fiber_start(attr, fct, ctx, 1);
  return *fiber ? 0 : -1;
}

const char * melon_fiber_name(const melon_fiber * fiber)
{
  return fiber->name;
}

void melon_fiber_setname(melon_fiber * fiber, const char * name)
{
  fiber->name = name;
}

void melon_fiber_join(melon_fiber * fiber, void ** retval)
{
  assert(fiber);
  melon_mutex_lock(fiber->join_mutex);
  if (!fiber->join_is_finished)
    melon_cond_wait(fiber->join_cond, fiber->join_mutex);
  else
    melon_cond_broadcast(fiber->join_cond);
  if (retval)
    *retval = fiber->join_retval;
  melon_mutex_unlock(fiber->join_mutex);
}

int melon_fiber_tryjoin(melon_fiber * fiber, void ** retval)
{
  assert(fiber);
  if (melon_mutex_trylock(fiber->join_mutex))
    goto busy;
  if (!fiber->join_is_finished)
  {
    melon_mutex_unlock(fiber->join_mutex);
    goto busy;
  }
  else
    melon_cond_broadcast(fiber->join_cond);
  if (retval)
    *retval = fiber->join_retval;
  melon_mutex_unlock(fiber->join_mutex);
  return 0;

  busy:
  errno = EBUSY;
  return -EBUSY;
}

int melon_fiber_timedjoin(melon_fiber * fiber, void ** retval, melon_time_t timeout)
{
  assert(fiber);
  if (melon_mutex_timedlock(fiber->join_mutex, timeout))
    goto timedout;
  if (!fiber->join_is_finished)
  {
    if (melon_cond_timedwait(fiber->join_cond, fiber->join_mutex, timeout))
    {
      melon_mutex_unlock(fiber->join_mutex);
      goto timedout;
    }
    else
      melon_cond_broadcast(fiber->join_cond);
  }
  else
    melon_cond_broadcast(fiber->join_cond);
  if (retval)
    *retval = fiber->join_retval;
  melon_mutex_unlock(fiber->join_mutex);
  return 0;

  timedout:
  errno = ETIMEDOUT;
  return -ETIMEDOUT;
}

void melon_fiber_detach(melon_fiber * fiber)
{
  assert(fiber);
  melon_mutex_lock(fiber->join_mutex);
  if (!fiber->join_is_finished)
    fiber->join_is_detached = 1;
  else
    melon_cond_broadcast(fiber->join_cond);
  melon_mutex_unlock(fiber->join_mutex);
}
