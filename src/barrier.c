#include <assert.h>
#include <stdlib.h>

#include "private.h"

melon_barrier * melon_barrier_new()
{
  melon_barrier * barrier = malloc(sizeof (*barrier));
  if (!barrier)
    return NULL;
  barrier->lock = melon_mutex_new();
  if (!barrier->lock)
    goto failure_mutex;
  barrier->cond = melon_cond_new();
  if (!barrier->cond)
    goto failure_cond;
  barrier->count = 0;
  return barrier;

  failure_cond:
  melon_mutex_destroy(barrier->lock);
  failure_mutex:
  free(barrier);
  return NULL;
}

void melon_barrier_destroy(melon_barrier * barrier)
{
  melon_mutex_lock(barrier->lock);
  //assert(barrier->count == 0);
  melon_mutex_unlock(barrier->lock);

  melon_cond_destroy(barrier->cond);
  melon_mutex_destroy(barrier->lock);
  free(barrier);
}

void melon_barrier_use(melon_barrier * barrier)
{
  __sync_add_and_fetch(&barrier->count, 1);
}

void melon_barrier_release(melon_barrier * barrier)
{
  if (__sync_add_and_fetch(&barrier->count, -1) == 0)
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
