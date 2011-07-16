#include <stdlib.h>
#include <assert.h>
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

void melon_cond_timedwait(melon_cond * condition, melon_mutex * mutex, uint64_t timeout)
{
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
    melon_dlist_pop(condition->wait_queue, fiber, );
    if (!fiber)
      break;
    melon_mutex_lock2(fiber, fiber->cond_mutex);
  }
}
