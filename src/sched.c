#include <assert.h>

#include "private.h"

__thread melon_fiber * g_current_fiber = NULL;
__thread ucontext_t    g_root_ctx;

static void sched_next()
{
  assert(!g_current_fiber);
  pthread_mutex_lock(&g_melon.mutex);
  while (!g_current_fiber)
  {
    if (g_melon.stop)
    {
      pthread_mutex_unlock(&g_melon.mutex);
      return;
    }

    melon_list_pop(g_melon.ready, g_current_fiber);
    if (!g_current_fiber)
    {
      pthread_cond_wait(&g_melon.ready_cond, &g_melon.mutex);
      continue;
    }
  }
  pthread_mutex_unlock(&g_melon.mutex);

  swapcontext(&g_root_ctx, &g_current_fiber->ctx);
}

void * melon_sched_run(void * dummy)
{
  (void)dummy;

  while (!g_melon.stop)
    sched_next();
  return NULL;
}

void melon_sched_next(void)
{
  melon_fiber * fiber = g_current_fiber;
  g_current_fiber = NULL;
  swapcontext(&fiber->ctx, &g_root_ctx);
}
