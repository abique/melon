#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "private.h"

struct melon g_melon = {0};

void melon_init(uint16_t nb_threads)
{
  if (nb_threads == 0)
    nb_threads = sysconf(_SC_NPROCESSORS_ONLN);
  assert(nb_threads > 0);
  g_melon.threads_nb = nb_threads;

  int ret = pthread_mutex_init(&g_melon.mutex, 0);
  assert(!ret);

  g_melon.ready_head = 0;
  g_melon.ready_tail = 0;

  /* maximize opened file descriptors */
  struct rlimit rl;
  unsigned int rc = getrlimit(RLIMIT_NOFILE, &rl);
  assert(!rc);

  rl.rlim_cur = rl.rlim_max;
  rc = setrlimit(RLIMIT_NOFILE, &rl);
  assert(!rc);

  /* allocate io_blocked */
  g_melon.io_blocked = calloc(1, sizeof (*g_melon.io_blocked) * rl.rlim_max);

  melon_io_manager_init();

  g_melon.threads = malloc(sizeof (*g_melon.threads) * g_melon.threads_nb);
  for (int i = 0; i < g_melon.threads_nb; ++i)
  {
    int ret = pthread_create(g_melon.threads + i, 0, melon_sched_run, 0);
    assert(!ret);
  }
}

void melon_deinit()
{
  melon_io_manager_deinit();
  for (int i = 0; i < g_melon.threads_nb; ++i)
    pthread_join(g_melon.threads[i], 0);
  free(g_melon.threads);
}
