#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "private.h"

int melon_semattr_init(melon_semattr ** attr)
{
  *attr = NULL;
  return 0;
}

void melon_semattr_destroy(melon_semattr * attr)
{
  free(attr);
}

int melon_sem_init(melon_sem ** sem, melon_semattr * attr, uint32_t nb)
{
  (void)attr;

  *sem = malloc(sizeof (**sem));
  if (!*sem)
    return -1;

  if (melon_mutex_init(&(*sem)->lock, NULL))
    goto failure_mutex;

  if (melon_cond_init(&(*sem)->cond, NULL))
    goto failure_cond;

  (*sem)->left  = nb;
  (*sem)->queue = NULL;
  return 0;

  failure_cond:
  melon_mutex_destroy((*sem)->lock);
  failure_mutex:
  free(*sem);
  return -1;
}

void melon_sem_destroy(melon_sem * sem)
{
  assert(sem->left == sem->nb);
  melon_cond_destroy(sem->cond);
  melon_mutex_destroy(sem->lock);
  free(sem);
}

void melon_sem_acquire(melon_sem * sem, uint32_t nb)
{
  int           in_queue = 0;
  melon_fiber * self     = melon_fiber_self();

  melon_mutex_lock(sem->lock);
  while (1)
  {
    if (sem->left >= nb)
    {
      sem->left -= nb;
      if (in_queue)
        melon_dlist_unlink(sem->queue, self, );
      melon_mutex_unlock(sem->lock);
      return;
    }

    if (!in_queue)
    {
      in_queue = 1;
      self->sem_nb = nb;
      melon_dlist_push(sem->queue, self, );
    }

    melon_cond_wait(sem->cond, sem->lock);
  }
}

void melon_sem_release(melon_sem * sem, uint32_t nb)
{
  melon_mutex_lock(sem->lock);
  sem->left += nb;
  assert(sem->left <= sem->nb);
  int left = sem->left;
  melon_fiber * curr = sem->queue;
  while (curr && curr->sem_nb > left)
  {
    melon_cond_signal(sem->cond);
    left -= curr->sem_nb;
    curr = curr->next;
    if (curr == sem->queue)
      break;
  }
  melon_mutex_unlock(sem->lock);
}

int melon_sem_tryacquire(melon_sem * sem, uint32_t nb)
{
  melon_mutex_lock(sem->lock);
  if (sem->left >= nb)
  {
    sem->left -= nb;
    melon_mutex_unlock(sem->lock);
    return 0;
  }
  melon_mutex_unlock(sem->lock);
  errno = EAGAIN;
  return -EAGAIN;
}

int melon_sem_timedacquire(melon_sem * sem, uint32_t nb, melon_time_t timeout)
{
  int           in_queue = 0;
  melon_fiber * self     = melon_fiber_self();

  if (melon_mutex_timedlock(sem->lock, timeout))
  {
    errno = ETIMEDOUT;
    return -ETIMEDOUT;
  }

  while (1)
  {
    if (sem->left >= nb)
    {
      sem->left -= nb;
      if (in_queue)
        melon_dlist_unlink(sem->queue, self, );
      melon_mutex_unlock(sem->lock);
      return 0;
    }

    if (!in_queue)
    {
      in_queue = 1;
      self->sem_nb = nb;
      melon_dlist_push(sem->queue, self, );
    }

    if (melon_cond_timedwait(sem->cond, sem->lock, timeout))
    {
      if (in_queue)
        melon_dlist_unlink(sem->queue, self, );
      melon_mutex_unlock(sem->lock);
      errno = ETIMEDOUT;
      return -ETIMEDOUT;
    }
  }
}
