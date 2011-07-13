#include <assert.h>

#include "private.h"

__thread melon_fiber * g_current_fiber = NULL;
__thread ucontext_t    g_root_ctx;
static __thread int    g_destroy_current_fiber = 0;

static void sched_next()
{
  assert(!g_current_fiber);
  int ret = pthread_mutex_lock(&g_melon.lock);
  assert(!ret);
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
      int ret = pthread_cond_wait(&g_melon.ready_cond, &g_melon.lock);
      assert(!ret);
      continue;
    }
  }
  pthread_mutex_unlock(&g_melon.lock);

  ret = swapcontext(&g_root_ctx, &g_current_fiber->ctx);
  assert(!ret);
}

void * melon_sched_run(void * dummy)
{
  (void)dummy;

  /* init TLS */
  g_current_fiber = NULL;
  g_destroy_current_fiber = 0;

  while (!g_melon.stop)
  {
    if (g_destroy_current_fiber)
    {
      assert(g_current_fiber);
      melon_fiber_destroy(g_current_fiber);
      g_current_fiber = NULL;
      g_destroy_current_fiber = 0;
    }
    sched_next();
  }
  return NULL;
}

void melon_sched_next(int destroy_current_fiber)
{
  melon_fiber * fiber = g_current_fiber;
  if (destroy_current_fiber)
    g_destroy_current_fiber = 1;
  else
  {
    g_current_fiber = NULL;
    g_destroy_current_fiber = 0;
  }
  int ret = swapcontext(&fiber->ctx, &g_root_ctx);
  assert(!ret);
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
