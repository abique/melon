#include <assert.h>
#include <time.h>

#include "private.h"

melon_time_t melon_time(void)
{
  struct timespec tp;
  int ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
  assert(!ret);
  return tp.tv_nsec + tp.tv_sec * 1000 * 1000 * 1000;
}

static void check_sleep_timeout(void)
{
}

static void check_io_timeout(void)
{
}

static void check_mutex_timeout(void)
{
}

static void check_rwmutex_timeout(void)
{
}

static void check_cond_timeout(void)
{
}

static void check_timeouts(void)
{
  check_sleep_timeout();
  check_io_timeout();
  check_mutex_timeout();
  check_rwmutex_timeout();
  check_cond_timeout();
}

void * melon_timer_manager_loop(void * dummy)
{
  (void)dummy;
  while (!g_melon.stop)
  {
    usleep(1000000);

    pthread_mutex_lock(&g_melon.lock);
    check_timeouts();
    pthread_mutex_unlock(&g_melon.lock);
  }
  return NULL;
}

int melon_timer_manager_init(void)
{
  if (pthread_create(&g_melon.timer_thread, NULL, melon_timer_manager_loop, NULL))
    return -1;
  return 0;
}

void melon_timer_manager_deinit(void)
{
  pthread_cancel(g_melon.timer_thread);
  pthread_join(g_melon.timer_thread, NULL);
}

void melon_timer_push(void)
{
  melon_fiber * fiber = melon_fiber_self();
  
}
