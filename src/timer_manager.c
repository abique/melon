#include <assert.h>
#include <time.h>

#include "private.h"

#define CLOCK_ID CLOCK_MONOTONIC_RAW

melon_time_t melon_time(void)
{
  struct timespec tp;
  int ret = clock_gettime(CLOCK_ID, &tp);
  assert(!ret);
  return tp.tv_nsec + tp.tv_sec * 1000 * 1000 * 1000;
}

void * melon_timer_manager_loop(void * dummy)
{
  (void)dummy;
  while (!g_melon.stop)
  {
    pthread_mutex_lock(&g_melon.lock);
    melon_time_t time = melon_time();
    melon_fiber * ready = NULL;
    melon_fiber * curr = g_melon.timer_queue;
    while (curr && curr->timer <= time)
    {
      g_melon.timer_queue = curr->timer_next;
      curr->timer_next = NULL;
      switch (curr->waited_event)
      {
      case kEventIoRead:
      case kEventIoWrite:
      case kEventTimeout:
      default:
        break;
      }
      melon_list_push(ready, curr, next);
    }
    melon_sched_ready_locked(ready);
    pthread_mutex_unlock(&g_melon.lock);
  }
  return NULL;
}

int melon_timer_manager_init(void)
{
  pthread_condattr_t cond_attr;
  if (pthread_condattr_init(&cond_attr))
    return -1;
  pthread_condattr_setclock(&cond_attr, CLOCK_ID);
  if (pthread_cond_init(&g_melon.timer_cond, &cond_attr))
  {
    pthread_condattr_destroy(&cond_attr);
    return -1;
  }
  pthread_condattr_destroy(&cond_attr);

  if (pthread_create(&g_melon.timer_thread, NULL, melon_timer_manager_loop, NULL))
  {
    pthread_cond_destroy(&g_melon.timer_cond);
    return -1;
  }
  return 0;
}

void melon_timer_manager_deinit(void)
{
  pthread_cancel(g_melon.timer_thread);
  pthread_join(g_melon.timer_thread, NULL);
  pthread_cond_destroy(&g_melon.timer_cond);
}

void melon_timer_push(void)
{
  melon_fiber * fiber = melon_fiber_self();
  melon_fiber * prev = NULL;
  melon_fiber * curr = NULL;
  pthread_mutex_lock(&g_melon.lock);
  curr = g_melon.timer_queue;
  while (curr)
  {
    if (fiber->timer <= curr->timer)
      break;
    prev = curr;
    curr = curr->timer_next;
  }
  if (!prev)
  {
    fiber->timer_next = g_melon.timer_queue;
    g_melon.timer_queue = fiber;
    if (!fiber->timer_next || fiber->timer != fiber->timer_next->timer)
      pthread_cond_signal(&g_melon.timer_cond);
  }
  else
  {
    fiber->timer_next = curr;
    prev->timer_next = fiber;
  }
  pthread_mutex_unlock(&g_melon.lock);
}
