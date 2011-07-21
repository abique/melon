#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "private.h"

melon_rwlock * melon_rwlock_new(void)
{
  melon_rwlock * rwlock = malloc(sizeof (*rwlock));
  if (!rwlock)
    goto failure_malloc;

  rwlock->lock = melon_mutex_new(0);
  if (!rwlock->lock)
    goto failure_mutex;
  rwlock->wcond = melon_cond_new();
  if (!rwlock->wcond)
    goto failure_cond1;
  rwlock->rcond = melon_cond_new();
  if (!rwlock->rcond)
    goto failure_cond2;
  rwlock->lock_count = 0;
  rwlock->wowner = NULL;
  rwlock->is_read = 0;
  return rwlock;

  failure_cond2:
  melon_cond_destroy(rwlock->wcond);
  failure_cond1:
  melon_mutex_destroy(rwlock->lock);
  failure_mutex:
  free(rwlock);
  failure_malloc:
  return NULL;
}

void melon_rwlock_destroy(melon_rwlock * rwlock)
{
  melon_cond_destroy(rwlock->rcond);
  melon_cond_destroy(rwlock->wcond);
  melon_mutex_destroy(rwlock->lock);
  free(rwlock);
}

void melon_rwlock_rdlock(melon_rwlock * rwlock)
{
  melon_mutex_lock(rwlock->lock);

  while (rwlock->lock_count > 0 && !rwlock->is_read)
    melon_cond_wait(rwlock->rcond, rwlock->lock);

  rwlock->is_read = 1;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
}

int melon_rwlock_tryrdlock(melon_rwlock * rwlock)
{
  melon_mutex_lock(rwlock->lock);

  if (rwlock->lock_count > 0 && !rwlock->is_read)
  {
    errno = EAGAIN;
    return -EAGAIN;
  }

  rwlock->is_read = 1;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
  return 0;
}

int melon_rwlock_timedrdlock(melon_rwlock * rwlock, uint64_t timeout)
{
  if (melon_mutex_timedlock(rwlock->lock, timeout))
  {
    errno = ETIMEDOUT;
    return -ETIMEDOUT;
  }

  while (rwlock->lock_count > 0 && !rwlock->is_read)
    if (melon_cond_timedwait(rwlock->rcond, rwlock->lock, timeout))
    {
      melon_mutex_unlock(rwlock->lock);
      errno = ETIMEDOUT;
      return -ETIMEDOUT;
    }

  rwlock->is_read = 1;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
  return 0;
}

void melon_rwlock_wrlock(melon_rwlock * rwlock)
{
  melon_mutex_lock(rwlock->lock);

  while (rwlock->wowner != g_current_fiber && rwlock->lock_count > 0)
    melon_cond_wait(rwlock->wcond, rwlock->lock);

  rwlock->wowner = g_current_fiber;
  rwlock->is_read = 0;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
}

int melon_rwlock_trywrlock(melon_rwlock * rwlock)
{
  melon_mutex_lock(rwlock->lock);

  if (rwlock->wowner != g_current_fiber && rwlock->lock_count > 0)
  {
    melon_mutex_unlock(rwlock->lock);
    errno = -EAGAIN;
    return -EAGAIN;
  }

  rwlock->wowner = g_current_fiber;
  rwlock->is_read = 0;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
  return 0;
}

int melon_rwlock_timedwrlock(melon_rwlock * rwlock, uint64_t timeout)
{
  if (melon_mutex_timedlock(rwlock->lock, timeout))
  {
    errno = -ETIMEDOUT;
    return -ETIMEDOUT;
  }

  while (rwlock->wowner != g_current_fiber && rwlock->lock_count > 0)
    if (melon_cond_timedwait(rwlock->wcond, rwlock->lock, timeout))
    {
      melon_mutex_unlock(rwlock->lock);
      errno = -ETIMEDOUT;
      return -ETIMEDOUT;
    }

  rwlock->wowner = g_current_fiber;
  rwlock->is_read = 0;
  assert(rwlock->lock_count + 1 > rwlock->lock_count);
  ++rwlock->lock_count;
  melon_mutex_unlock(rwlock->lock);
  return 0;
}

void melon_rwlock_unlock(melon_rwlock * rwlock)
{
  melon_mutex_lock(rwlock->lock);
  if (--rwlock->lock_count == 0)
  {
    rwlock->wowner = NULL;
    if (rwlock->wcond)
      melon_cond_signal(rwlock->wcond);
    else if (rwlock->rcond)
      melon_cond_broadcast(rwlock->rcond);
  }
  melon_mutex_unlock(rwlock->lock);
}
