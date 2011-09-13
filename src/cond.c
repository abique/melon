#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "private.h"

int melon_condattr_init(melon_condattr ** attr)
{
  *attr = NULL;
  return 0;
}

void melon_condattr_destroy(melon_condattr * attr)
{
  (void)attr;
}

int melon_cond_init(melon_cond ** cond, melon_condattr * attr)
{
  (void)attr;
  *cond = malloc(sizeof (*cond));
  if (!*cond)
    return -1;
  melon_spin_init(&(*cond)->lock);
  (*cond)->wait_queue = NULL;
  return 0;
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

  /* save the mutex to relock and its lock count */
  g_current_fiber->cond_mutex = mutex;
  self->lock_count            = lock_count;
  self->timer                 = 0;

  /* push the fiber in the wait queue */
  melon_spin_lock(&condition->lock);
  melon_dlist_push(condition->wait_queue, self, );
  melon_spin_unlock(&condition->lock);

  /* unlock the mutex */
  mutex->lock_count = 1;
  melon_mutex_unlock(mutex);

  /* wait for the wake up */
  melon_sched_next();
}

struct wait_ctx
{
  melon_fiber * fiber;
  melon_cond *  cond;
  int           timeout;
  int           debug;
};

static void melon_cond_timedwait_cb(struct wait_ctx * ctx)
{
  assert(ctx);
  assert(ctx->debug == 0);
  ++ctx->debug;

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
  if (timeout == 0)
  {
    melon_cond_wait(condition, mutex);
    return 0;
  }

  melon_fiber * self = g_current_fiber;

  /* ensure that the mutex is locked */
  assert(condition);
  assert(mutex);
  assert(mutex->owner == self);

  /* save the lock count */
  int lock_count = mutex->lock_count;

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
  ctx.debug       = 0;
  self->timer     = timeout;
  self->timer_cb  = (melon_callback)melon_cond_timedwait_cb;
  self->timer_ctx = &ctx;
  melon_timer_push_locked();

  /* unlock */
  melon_spin_unlock(&condition->lock);
  pthread_mutex_unlock(&g_melon.lock);

  /* unlock the mutex */
  mutex->lock_count = 1;
  melon_mutex_unlock(mutex);

  /* wait for the wake up */
  melon_sched_next();

  /* check timeout */
  if (!ctx.timeout)
    return 0;
  errno = ETIMEDOUT;
  return -ETIMEDOUT;
}

// TODO use pthread only for timed waits
int melon_cond_signalnb(melon_cond * condition, const uint32_t nb)
{
  pthread_mutex_lock(&g_melon.lock);
  melon_spin_lock(&condition->lock);
  melon_fiber * list    = condition->wait_queue;
  condition->wait_queue = NULL;
  uint32_t nwokeup      = 0;

  melon_fiber * fiber = NULL;
  while (nb == 0 || nwokeup < nb)
  {
    melon_dlist_pop(list, fiber, );
    if (!fiber)
      break;
    ++nwokeup;
    if (fiber->timer > 0)
      melon_timer_remove_locked(fiber);
    melon_mutex_lock2(fiber, fiber->cond_mutex);
  }
  melon_spin_unlock(&condition->lock);
  pthread_mutex_unlock(&g_melon.lock);
  return nwokeup;
}

int melon_cond_signal(melon_cond * condition)
{
  return melon_cond_signalnb(condition, 1);
}

int melon_cond_broadcast(melon_cond * condition)
{
  return melon_cond_signalnb(condition, 0);
}
