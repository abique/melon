#include <assert.h>
#include <stdlib.h>

#include "private.h"

int melon_barrierattr_init(melon_barrierattr ** attr)
{
  *attr = NULL;
  return 0;
}

void melon_barrierattr_destroy(melon_barrierattr * attr)
{
  free(attr);
}

int melon_barrier_init(melon_barrier ** barrier, melon_barrierattr * attr, uint32_t nb)
{
  (void)attr;
  *barrier = malloc(sizeof (**barrier));
  if (!(*barrier))
    return -1;
  if (melon_mutex_init(&(*barrier)->lock, NULL))
    goto failure_mutex;
  if (melon_cond_init(&(*barrier)->cond, NULL))
    goto failure_cond;
  (*barrier)->count = nb;
  return 0;

  failure_cond:
  melon_mutex_destroy((*barrier)->lock);
  failure_mutex:
  free(*barrier);
  return -1;
}

void melon_barrier_destroy(melon_barrier * barrier)
{
  /* lock before destroy to be sure that nobody is holding the lock */
  melon_mutex_lock(barrier->lock);
  melon_mutex_unlock(barrier->lock);

  melon_cond_destroy(barrier->cond);
  melon_mutex_destroy(barrier->lock);
  free(barrier);
}

void melon_barrier_add(melon_barrier * barrier, uint32_t nb)
{
  __sync_add_and_fetch(&barrier->count, nb);
}

void melon_barrier_release(melon_barrier * barrier, uint32_t nb)
{
  assert(nb <= barrier->count);
  if (__sync_add_and_fetch(&barrier->count, -nb) == 0)
  {
    melon_mutex_lock(barrier->lock);
    melon_cond_broadcast(barrier->cond);
    melon_mutex_unlock(barrier->lock);
  }
}

void melon_barrier_wait(melon_barrier * barrier)
{
  melon_mutex_lock(barrier->lock);
  if (__sync_add_and_fetch(&barrier->count, -1) == 0)
    melon_cond_broadcast(barrier->cond);
  else
    melon_cond_wait(barrier->cond, barrier->lock);
  __sync_add_and_fetch(&barrier->count, 1);
  melon_mutex_unlock(barrier->lock);
}
