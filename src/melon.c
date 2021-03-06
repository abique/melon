#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <limits.h>

#include "private.h"

struct melon g_melon;

int melon_init(uint32_t nb_threads)
{
  uint32_t i = 0;

  /* default values */
  g_melon.stop = 0;
  g_melon.fibers_count = 0;
  g_melon.ready = NULL;

  pthread_mutexattr_t mutex_attr;
  if (pthread_mutexattr_init(&mutex_attr))
    return -1;

  if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE))
    goto failure_mutex;

  /* init the mutex */
  if (pthread_mutex_init(&g_melon.lock, &mutex_attr))
    goto failure_mutex;

  pthread_mutexattr_destroy(&mutex_attr);

  if (pthread_cond_init(&g_melon.ready_cond, NULL))
    goto failure_cond;

  if (pthread_cond_init(&g_melon.fibers_count_zero, NULL))
    goto failure_cond2;

  melon_stack_init();

  /* get the number of threads in the thread pull */
  if (nb_threads == 0)
    nb_threads = sysconf(_SC_NPROCESSORS_ONLN);
  g_melon.threads_nb = nb_threads > 0 ? nb_threads : 1;

  /* get the limit of opened file descriptors */
  int rlimit_nofile = _POSIX_OPEN_MAX;
  struct rlimit rl;
  if (!getrlimit(RLIMIT_NOFILE, &rl))
    rlimit_nofile = rl.rlim_max;

  /* allocate io_blocked */
  g_melon.io = calloc(1, sizeof (*g_melon.io) * rlimit_nofile);
  if (!g_melon.io)
    goto failure_calloc;

  /* initialize io manager */
  if (melon_io_manager_init())
    goto failure_io_manager;

  /* initialize timer manager */
  if (melon_timer_manager_init())
    goto failure_timer_manager;

  /* allocates thread pool */
  g_melon.threads = malloc(sizeof (*g_melon.threads) * g_melon.threads_nb);
  for (i = 0; i < g_melon.threads_nb; ++i)
    if (pthread_create(g_melon.threads + i, 0, melon_sched_run, 0))
      goto failure_pthread;
  /* succeed */
  return 0;

  failure_pthread:
  g_melon.stop = 1;
  /* join thread pool and free threads */
  for (; i > 0; --i)
    pthread_join(g_melon.threads[i - 1], NULL);
  free(g_melon.threads);
  g_melon.threads = NULL;
  melon_timer_manager_deinit();

  failure_timer_manager:
  /* deinit io manager */
  melon_io_manager_deinit();

  failure_io_manager:
  /* free io_blocked */
  free(g_melon.io);
  g_melon.io = NULL;

  failure_calloc:
  pthread_cond_destroy(&g_melon.fibers_count_zero);

  failure_cond2:
  pthread_cond_destroy(&g_melon.ready_cond);

  failure_cond:
  pthread_mutex_destroy(&g_melon.lock);

  failure_mutex:
  pthread_mutexattr_destroy(&mutex_attr);
  return -1;
}

void melon_deinit()
{
  // TODO check the number of fibers and warn if > 0
  assert(g_melon.fibers_count == 0);

  pthread_mutex_lock(&g_melon.lock);
  g_melon.stop = 1;
  pthread_cond_broadcast(&g_melon.ready_cond);
  pthread_mutex_unlock(&g_melon.lock);

  /* io_manager */
  melon_io_manager_deinit();

  /* timer_manager */
  melon_timer_manager_deinit();

  /* thread pool */
  for (unsigned i = 0; i < g_melon.threads_nb; ++i)
    pthread_join(g_melon.threads[i], 0);
  free(g_melon.threads);
  g_melon.threads = NULL;

  melon_stack_deinit();

  pthread_cond_destroy(&g_melon.fibers_count_zero);
  pthread_cond_destroy(&g_melon.ready_cond);
  pthread_mutex_destroy(&g_melon.lock);

  /* io_blocked */
  free(g_melon.io);
  g_melon.io = NULL;
}

void melon_wait()
{
  pthread_mutex_lock(&g_melon.lock);
  while (1)
  {
    assert(g_melon.fibers_count >= 0);
    if (g_melon.fibers_count == 0)
      break;
    pthread_cond_wait(&g_melon.fibers_count_zero, &g_melon.lock);
  }
  pthread_mutex_unlock(&g_melon.lock);
}
