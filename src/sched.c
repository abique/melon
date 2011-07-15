#include <assert.h>

#include "private.h"

__thread melon_fiber * g_current_fiber = NULL;
__thread ucontext_t    g_root_ctx;

static void sched_next()
{
  g_current_fiber = NULL;
  int ret = pthread_mutex_lock(&g_melon.lock);
  assert(!ret);
  while (!g_current_fiber)
  {
    if (g_melon.stop)
    {
      pthread_mutex_unlock(&g_melon.lock);
      return;
    }

    melon_list_pop(g_melon.ready, g_current_fiber, next);
    if (!g_current_fiber)
    {
      int ret = pthread_cond_wait(&g_melon.ready_cond, &g_melon.lock);
      assert(!ret);
      continue;
    }
  }
  pthread_mutex_unlock(&g_melon.lock);

  melon_spin_lock(&g_current_fiber->lock);
  ret = swapcontext(&g_root_ctx, &g_current_fiber->ctx);
  assert(!ret);
  melon_spin_unlock(&g_current_fiber->lock);
}

void * melon_sched_run(void * dummy)
{
  (void)dummy;

  /* init TLS */
  g_current_fiber = NULL;

  while (!g_melon.stop)
  {
    sched_next();
    if (g_current_fiber && g_current_fiber->sched_next_cb)
    {
      // do a copy because cb(ctx) may destroy the fiber and we wants
      melon_callback cb  = g_current_fiber->sched_next_cb;
      void *         ctx = g_current_fiber->sched_next_ctx;
      g_current_fiber->sched_next_cb  = NULL;
      g_current_fiber->sched_next_ctx = NULL;
      cb(ctx);
    }
  }
  return NULL;
}

void melon_sched_next()
{
  assert(g_current_fiber);
  int ret = swapcontext(&g_current_fiber->ctx, &g_root_ctx);
  assert(!ret);
}

void melon_sched_ready_locked(melon_fiber * list)
{
  melon_fiber * fiber = NULL;
  int nb = 0;

  while (1)
  {
    melon_list_pop(list, fiber, next);
    if (!fiber)
      break;
    fiber->waited_event = kEventNone;
    melon_list_push(g_melon.ready, fiber, next);
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
