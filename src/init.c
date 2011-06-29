#include <assert.h>
#include <stdlib.h>

#include "private.h"

struct melon g_melon = {0};

void melon_init(int nb_threads)
{
  assert(nb_threads >= 1);
  melon_io_manager_init(&g_melon.io_manager);

  int ret = pthread_mutex_init(&g_melon.events_mutex, 0);
  assert(!ret);

  int ret = pthread_cond_init(&g_melon.events_cond, 0);
  assert(!ret);
  g_melon.events_head = 0;
  g_melon.events_tail = 0;

  g_melon.threads = malloc(sizeof (*g_melon.threads) * nb_threads);
  for (int i = 0; i < nb_threads; ++i)
  {
    int ret = pthread_create(g_melon.threads + i, 0, melon_sched_run, 0);
    assert(!ret);
  }
}

void melon_deinit()
{
  melon_io_manager_deinit(&g_melon.io_manager);
  free(g_melon.threads);
}
