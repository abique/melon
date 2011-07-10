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

int melon_timer_manager_init(void)
{
  g_melon.time_func = melon_time_default;
  g_melon.time_ctx = NULL;
  g_melon.timer_resolution = 100 * 1000; // 100us
  if (pthread_create(&g_melon.timer_thread, NULL, melon_timer_manager_loop, NULL))
    return -1;
  return 0;
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
  check_io_timeout();
  check_mutex_timeout();
  check_rwmutex_timeout();
  check_cond_timeout();
}

static void destroy_fibers()
{
}

void * melon_timer_manager_loop(void * dummy)
{
  (void)dummy;
  while (!g_melon.stop)
  {
    pthread_mutex_lock(&g_melon.mutex);
    check_timeouts();
    destroy_fibers();
    pthread_mutex_unlock(&g_melon.mutex);

    usleep(g_melon.timer_resolution / 1000);
  }
  return NULL;
}

void melon_timer_manager_deinit(void)
{
}
