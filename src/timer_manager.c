#include <assert.h>
#include <time.h>

#include "private.h"

#define CLOCK_ID CLOCK_MONOTONIC

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
  pthread_mutex_lock(&g_melon.lock);
  while (!g_melon.stop)
  {
    melon_time_t  time  = melon_time();
    melon_fiber * curr  = g_melon.timer_queue;
    while (curr && curr->timer <= time)
    {
      g_melon.timer_queue = curr->timer_next;
      curr->timer_next = NULL;
      assert(curr->timer_cb);
      curr->timer_cb(curr->timer_ctx);
      curr = g_melon.timer_queue;
    }
    if (g_melon.timer_queue)
    {
      struct timespec tp;
      tp.tv_sec = g_melon.timer_queue->timer / 1000 / 1000 / 1000;
      tp.tv_nsec = g_melon.timer_queue->timer % (1000 * 1000 * 1000);
      pthread_cond_timedwait(&g_melon.timer_cond, &g_melon.lock, &tp);
    }
    else
      pthread_cond_wait(&g_melon.timer_cond, &g_melon.lock);
  }
  pthread_mutex_unlock(&g_melon.lock);
  return NULL;
}

int melon_timer_manager_init(void)
{
  g_melon.timer_queue = NULL;
  pthread_condattr_t cond_attr;
  if (pthread_condattr_init(&cond_attr))
    return -1;
  int ret = pthread_condattr_setclock(&cond_attr, CLOCK_ID);
  assert(!ret);
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
  pthread_cond_signal(&g_melon.timer_cond);
  pthread_join(g_melon.timer_thread, NULL);
  pthread_cond_destroy(&g_melon.timer_cond);
}

void melon_timer_push(void)
{
  melon_fiber * fiber = melon_fiber_self();
  melon_fiber * prev = NULL;
  melon_fiber * curr = NULL;
  fiber->timer_next = NULL;
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
  fiber->sched_next_cb  = (void(*)(void*))pthread_mutex_unlock;
  fiber->sched_next_ctx = &g_melon.lock;
  melon_sched_next();
}

unsigned int melon_sleep(uint32_t secs)
{
  return melon_usleep(secs * 1000 * 1000);
}

static void melon_usleep_cb(melon_fiber * fiber)
{
  melon_sched_ready_locked(fiber);
}

int melon_usleep(uint64_t usecs)
{
  melon_fiber * self = melon_fiber_self();
  self->waited_event = kEventTimer;
  self->timer = melon_time() + usecs * 1000;
  self->timer_cb = (melon_callback)melon_usleep_cb;
  self->timer_ctx = self;
  melon_timer_push();
  return 0;
}
