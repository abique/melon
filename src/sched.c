#include "private.h"

void * melon_sched_run(void * dummy)
{
  (void)dummy;

  while (!g_melon.stop && melon_sched_next())
    continue;
  return NULL;
}

int melon_sched_next(void)
{
  melon_fiber * fiber = NULL;
  while (!fiber)
  {
    if (g_melon.stop)
      return 0;

    melon_list_pop(g_melon.ready, fiber);
    if (!fiber)
    {
      pthread_mutex_lock(&g_melon.mutex);
      pthread_cond_wait(&g_melon.ready_cond, &g_melon.mutex);
      pthread_mutex_unlock(&g_melon.mutex);
    }
  }

  /* swapcontext(); */
  /* melon_fiber * self = melon_self_fiber(); */
  return 1;
}
