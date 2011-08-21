#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "private.h"

melon_mutex * melon_mutex_new(int is_recursive)
{
  melon_mutex * mutex = malloc(sizeof (*mutex));
  if (!mutex)
    return NULL;
  melon_spin_init(&mutex->lock);
  mutex->owner        = NULL;
  mutex->lock_queue   = NULL;
  mutex->is_recursive = is_recursive;
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

void melon_mutex_lock2(melon_fiber * fiber, melon_mutex * mutex)
{
  assert(fiber);
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  fiber->timer = 0;
  if (!mutex->owner)
  {
    mutex->owner      = fiber;
    mutex->lock_count = fiber->lock_count; // for cond_wait, restoring the lock count
    assert(mutex->lock_count == 1 || (mutex->lock_count > 1 && mutex->is_recursive));
    melon_spin_unlock(&mutex->lock);
    melon_sched_ready(fiber);
    return;
  }
  else
  {
    assert(mutex->owner != fiber);
    melon_dlist_push(mutex->lock_queue, fiber, );
    melon_spin_unlock(&mutex->lock);
  }
}

void melon_mutex_lock(struct melon_mutex * mutex)
{
  melon_fiber * self = melon_fiber_self();
  assert(self && "you can't lock melon mutex out of an initialized thread");
  self->timer        = 0;
  self->lock_count   = 1; // Initialise the lock count, see melon_cond_wait
                          // to understand. Not used while relocking recursive
                          // mutex.
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  self->timer = 0;
  if (!mutex->owner)
  {
    mutex->owner = self;
    mutex->lock_count = 1; // for condition restoring the lock count
    melon_spin_unlock(&mutex->lock);
    return;
  }
  else if (mutex->owner == self)
  {
    if (!mutex->is_recursive)
    {
      assert(0 && "logic error, relocking non-recursive mutex");
      mutex->is_recursive = 1;
    }
    ++mutex->lock_count;
    melon_spin_unlock(&mutex->lock);
  }
  else
  {
    melon_dlist_push(mutex->lock_queue, self, );
    melon_spin_unlock(&mutex->lock);
    melon_sched_next();
  }
}

void melon_mutex_unlock(struct melon_mutex * mutex)
{
  assert(mutex);
  assert(mutex->lock_count > 0);

  if (--mutex->lock_count > 0)
    return;

  melon_spin_lock(&mutex->lock);
  if (!mutex->lock_queue)
  {
    mutex->owner = NULL;
    melon_spin_unlock(&mutex->lock);
    return;
  }
  melon_spin_unlock(&mutex->lock);

  pthread_mutex_lock(&g_melon.lock);
  melon_spin_lock(&mutex->lock);
  melon_dlist_pop(mutex->lock_queue, mutex->owner, );
  if (mutex->owner)
  {
    if (mutex->owner->timer > 0)
      melon_timer_remove_locked(mutex->owner);
    ++mutex->lock_count;
    melon_sched_ready_locked(mutex->owner);
  }
  melon_spin_unlock(&mutex->lock);
  pthread_mutex_unlock(&g_melon.lock);
}

int melon_mutex_trylock(struct melon_mutex * mutex)
{
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  if (!mutex->owner ||
      (mutex->is_recursive && mutex->owner == melon_fiber_self()))
  {
    mutex->owner = melon_fiber_self();
    ++mutex->lock_count;
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
  melon_sched_ready_locked(ctx->fiber);
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
    ++mutex->lock_count;
    melon_spin_unlock(&mutex->lock);
    pthread_mutex_unlock(&g_melon.lock);
    return 0;
  }

  struct lock_ctx ctx;
  ctx.mutex   = mutex;
  ctx.fiber   = g_current_fiber;
  ctx.timeout = 0;

  melon_fiber * self = melon_fiber_self();
  melon_dlist_push(mutex->lock_queue, self, );
  self->timer        = timeout;
  self->timer_cb     = (melon_callback)melon_mutex_timedlock_cb;
  self->timer_ctx    = &ctx;

  melon_timer_push_locked();
  melon_spin_unlock(&mutex->lock);
  pthread_mutex_unlock(&g_melon.lock);

  /* check timeout */
  melon_sched_next();
  if (!ctx.timeout)
    return 0;
  errno = ETIMEDOUT;
  return -ETIMEDOUT;
}
