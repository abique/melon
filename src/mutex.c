#include <assert.h>
#include "private.h"

melon_mutex * melon_mutex_new(void)
{
  melon_mutex * mutex = malloc(sizeof (*mutex));
  if (!mutex)
    return NULL;
  melon_spin_init(&mutex->lock);
  mutex->owner = NULL;
  mutex->lock_queue = NULL;
  mutex->is_recursive = 0;
  mutex->lock_count = 0;
  return mutex;
}

void melon_mutex_destroy(struct melon_mutex * mutex)
{
  melon_spinlock(&mutex->lock);
  assert(!lock_count);
  assert(!owner);
  assert(!lock_queue);
  free(mutex);
}

void melon_mutex_lock(struct melon_mutex * mutex)
{
  assert(mutex);
  melon_spin_lock(&mutex->lock);
  melon_fiber * self = melon_fiber_self();
  if (mutex->owner == self)
  {
    if (!is_recursive)
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
    melon_sched_ready(mutex->owner);
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

int melon_mutex_timedlock(struct melon_mutex * mutex, uint64_t timeout);
