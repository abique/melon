#include <assert.h>

#include "private.h"

__thread melon_fiber * g_current_fiber = NULL;
__thread ucontext_t    g_root_ctx;

static void destroy_fibers()
{
  pthread_spin_lock(&g_melon.destroy_lock);
  melon_fiber * list = g_melon.destroy;
  g_melon.destroy = NULL;
  pthread_spin_unlock(&g_melon.destroy_lock);
  if (list)
    while (1)
    {
      melon_fiber * fiber = NULL;
      melon_list_pop(list, fiber);
      if (!fiber)
        break;
      melon_fiber_destroy(fiber);
    }
}

static void sched_next()
{
  assert(!g_current_fiber);
  pthread_mutex_lock(&g_melon.lock);
  while (!g_current_fiber)
  {
    if (g_melon.stop)
    {
      pthread_mutex_unlock(&g_melon.lock);
      return;
    }

    melon_list_pop(g_melon.ready, g_current_fiber);
    if (!g_current_fiber)
    {
      pthread_cond_wait(&g_melon.ready_cond, &g_melon.lock);
      continue;
    }
  }
  pthread_mutex_unlock(&g_melon.lock);

  swapcontext(&g_root_ctx, &g_current_fiber->ctx);
}

void * melon_sched_run(void * dummy)
{
  (void)dummy;

  while (!g_melon.stop)
  {
    destroy_fibers();
    sched_next();
  }
  destroy_fibers();
  return NULL;
}

void melon_sched_next(void)
{
  melon_fiber * fiber = g_current_fiber;
  g_current_fiber = NULL;
  swapcontext(&fiber->ctx, &g_root_ctx);
}

void melon_sched_ready_locked(melon_fiber * list)
{
  melon_fiber * fiber = NULL;
  int nb = 0;

  while (1)
  {
    melon_list_pop(list, fiber);
    if (!fiber)
      break;
    melon_list_push(g_melon.ready, fiber);
    ++nb;
  }

  if (nb > 1)
    pthread_cond_broadcast(&g_melon.ready_cond);
  else if (nb == 1)
    pthread_cond_signal(&g_melon.ready_cond);
}

void melon_sched_ready(melon_fiber * list)
{
  pthread_mutex_lock(&g_melon.lock);
  melon_sched_ready_locked(list);
  pthread_mutex_unlock(&g_melon.lock);
}
