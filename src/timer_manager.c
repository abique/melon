#include <assert.h>
#include <time.h>

#include "private.h"

static melon_time_t melon_time_default(void * ctx)
{
  (void)ctx;
  struct timespec tp;
  int ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
  assert(!ret);
  return tp.tv_nsec + tp.tv_sec * 1000 * 1000 * 1000;
}

static void check_sleep_timeout()
{
}

static void check_io_timeout()
{
}

static void check_mutex_timeout()
{
}

static void check_rwmutex_timeout()
{
}

static void check_cond_timeout()
{
}

static void check_timeouts()
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
    usleep(g_melon.timer_resolution / 1000);

    pthread_mutex_lock(&g_melon.lock);
    check_timeouts();
    pthread_mutex_unlock(&g_melon.lock);
  }
  return NULL;
}

int melon_timer_manager_init(void)
{
  g_melon.time_func = melon_time_default;
  g_melon.time_ctx = NULL;
  g_melon.timer_resolution = 10 * 1000 * 1000; // 10ms
  if (pthread_create(&g_melon.timer_thread, NULL, melon_timer_manager_loop, NULL))
    return -1;
  return 0;
}

void melon_timer_manager_deinit(void)
{
  pthread_cancel(g_melon.timer_thread);
  pthread_join(g_melon.timer_thread, NULL);
}
