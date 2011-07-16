#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "private.h"

melon_cond * melon_cond_new(void)
{
  melon_cond * cond = malloc(sizeof (*cond));
  melon_spin_init(&cond->lock);
  cond->wait_queue = NULL;
  return cond;
}

void melon_cond_destroy(melon_cond * condition)
{
  melon_spin_lock(&condition->lock);
  assert(!condition->wait_queue);
  free(condition);
}

void melon_cond_wait(melon_cond * condition, melon_mutex * mutex)
{
  melon_fiber * self = g_current_fiber;

  /* ensure that the mutex is locked */
  assert(condition);
  assert(mutex);
  assert(mutex->owner == self);

  /* save the lock count */
  int lock_count = mutex->lock_count;

  /* unlock the mutex */
  mutex->lock_count = 1;
  melon_mutex_unlock(mutex);

  /* save the mutex to relock and its lock count */
  g_current_fiber->cond_mutex = mutex;
  self->lock_count = lock_count;

  /* push the fiber in the wait queue */
  melon_spin_lock(&condition->lock);
  melon_dlist_push(condition->wait_queue, self, );
  melon_spin_unlock(&condition->lock);

  /* wait for the wake up */
  melon_sched_next();
}

struct wait_ctx
{
  melon_fiber * fiber;
  melon_cond *  cond;
  int           timeout;
};

static void melon_cond_timedwait_cb(struct wait_ctx * ctx)
{
  melon_spin_lock(&ctx->cond->lock);
  // remove from the lock_queue
  melon_dlist_unlink(ctx->cond->wait_queue, ctx->fiber, );
  melon_spin_unlock(&ctx->cond->lock);
  ctx->timeout = 1;

  // lock the mutex
  melon_mutex_lock2(ctx->fiber, ctx->fiber->cond_mutex);
}

int melon_cond_timedwait(melon_cond * condition, melon_mutex * mutex, uint64_t timeout)
{
  melon_fiber * self = g_current_fiber;

  /* ensure that the mutex is locked */
  assert(condition);
  assert(mutex);
  assert(mutex->owner == self);

  /* save the lock count */
  int lock_count = mutex->lock_count;

  /* unlock the mutex */
  mutex->lock_count = 1;
  melon_mutex_unlock(mutex);

  /* save the mutex to relock and its lock count */
  g_current_fiber->cond_mutex = mutex;
  self->lock_count            = lock_count;

  /* lock */
  pthread_mutex_lock(&g_melon.lock);
  melon_spin_lock(&condition->lock);

  /* push the fiber in the wait queue */
  melon_dlist_push(condition->wait_queue, self, );

  /* setup the timeout */
  struct wait_ctx ctx;
  ctx.fiber       = self;
  ctx.cond        = condition;
  ctx.timeout     = 0;
  self->timer     = timeout;
  self->timer_cb  = (melon_callback)melon_cond_timedwait_cb;
  self->timer_ctx = &ctx;
  melon_timer_push_locked();

  /* unlock */
  melon_spin_unlock(&condition->lock);
  pthread_mutex_unlock(&g_melon.lock);

  /* wait for the wake up */
  melon_sched_next();

  /* check timeout */
  if (!ctx.timeout)
    return 0;
  errno = ETIMEDOUT;
  return -ETIMEDOUT;
}

void melon_cond_signal(melon_cond * condition)
{
  melon_spin_lock(&condition->lock);
  melon_fiber * fiber = NULL;
  melon_dlist_pop(condition->wait_queue, fiber, );
  melon_spin_unlock(&condition->lock);
  if (fiber)
    melon_mutex_lock2(fiber, fiber->cond_mutex);
}

void melon_cond_broadcast(melon_cond * condition)
{
  melon_spin_lock(&condition->lock);
  melon_fiber * list    = condition->wait_queue;
  condition->wait_queue = NULL;
  melon_spin_unlock(&condition->lock);

  melon_fiber * fiber = NULL;
  while (1)
  {
    melon_dlist_pop(list, fiber, );
    if (!fiber)
      break;
    melon_mutex_lock2(fiber, fiber->cond_mutex);
  }
}
