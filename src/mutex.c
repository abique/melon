#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "private.h"

melon_mutex * melon_mutex_new(void)
{
  melon_mutex * mutex = malloc(sizeof (*mutex));
  if (!mutex)
    return NULL;
  melon_spin_init(&mutex->lock);
  mutex->owner        = NULL;
  mutex->lock_queue   = NULL;
  mutex->is_recursive = 0;
  mutex->lock_count   = 0;
  return mutex;
}

void melon_mutex_destroy(struct melon_mutex * mutex)
{
  melon_spin_lock(&mutex->lock);
  assert(!mutex->lock_count);
  assert(!mutex->owner);
  assert(!mutex->lock_queue);
  free(mutex);
}

void melon_mutex_lock(struct melon_mutex * mutex)
{
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  melon_fiber * self = melon_fiber_self();
  self->timer = 0;
  if (mutex->owner == self)
  {
    if (!mutex->is_recursive)
      assert(0 && "logic error, relocking non-recursive mutex");
    ++mutex->lock_count;
    mutex->is_recursive = 2;
    melon_spin_unlock(&mutex->lock);
  }
  else
  {
    melon_list_push(mutex->lock_queue, self, next);
    self->sched_next_cb  = (melon_callback)melon_spin_unlock;
    self->sched_next_ctx = &mutex->lock;
    melon_sched_next();
  }
}

void melon_mutex_unlock(struct melon_mutex * mutex)
{
  assert(mutex);

  if (mutex->is_recursive)
    if (--mutex->lock_count > 0)
      return;

  melon_spin_lock(&mutex->lock);
  if (!mutex->lock_queue)
  {
    mutex->owner = NULL;
    return;
  }
  melon_spin_unlock(&mutex->lock);

  pthread_mutex_lock(&g_melon.lock);
  melon_spin_lock(&mutex->lock);
  melon_list_pop(mutex->lock_queue, mutex->owner, next);
  if (mutex->owner)
  {
    if (mutex->owner->timer)
      melon_timer_remove_locked(mutex->owner);
    melon_sched_ready(mutex->owner);
  }
  melon_spin_unlock(&mutex->lock);
  pthread_mutex_unlock(&g_melon.lock);
}

int melon_mutex_trylock(struct melon_mutex * mutex)
{
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  if (!mutex->owner)
  {
    mutex->owner = melon_fiber_self();
    melon_spin_unlock(&mutex->lock);
    return 0;
  }
  melon_spin_unlock(&mutex->lock);
  errno = EBUSY;
  return EBUSY;
}

struct lock_ctx
{
  melon_mutex * mutex;
  melon_fiber * fiber;
  int           timeout;
};

static void melon_mutex_timedlock_cb(struct lock_ctx * ctx)
{
  melon_spin_lock(&ctx->mutex->lock);
  // remove from the lock_queue
  melon_dlist_unlink(ctx->mutex->lock_queue, ctx->fiber, );
  melon_spin_unlock(&ctx->mutex->lock);
  ctx->timeout = 1;
}

int melon_mutex_timedlock(melon_mutex * mutex, uint64_t timeout)
{
  // lets do a first spinlock only try
  if (!melon_mutex_trylock(mutex))
    return 0;

  // ok now real stuff, we have to setup timeout and the queue
  pthread_mutex_lock(&g_melon.lock);
  melon_spin_lock(&mutex->lock);

  // just re-check owner
  if (!mutex->owner)
  {
    mutex->owner = melon_fiber_self();
    melon_spin_unlock(&mutex->lock);
    pthread_mutex_unlock(&g_melon.lock);
    return 0;
  }

  struct lock_ctx ctx;
  ctx.mutex   = mutex;
  ctx.fiber   = g_current_fiber;
  ctx.timeout = 0;

  melon_fiber * self = melon_fiber_self();
  melon_list_push(mutex->lock_queue, self, next);
  self->timer        = timeout;
  self->timer_cb     = melon_mutex_timedlock_cb;
  self->timer_ctx    = &ctx;

  melon_spin_unlock(&mutex->lock);
  pthread_mutex_unlock(&g_melon.lock);
  melon_sched_next();
  if (!ctx.timeout)
    return 0;
  errno = ETIMEDOUT;
  return -ETIMEDOUT;
}
