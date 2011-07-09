#ifndef CONFIG_MMAP_ALLOW_UNINITIALIZED
# define CONFIG_MMAP_ALLOW_UNINITIALIZED
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "private.h"

#ifndef MAP_UNINITIALIZED
# define MAP_UNINITIALIZED 0x0
#endif

melon_fiber * melon_fiber_self(void)
{
  return g_current_fiber;
}

melon_fiber * melon_fiber_start(void (*fct)(void *), void * ctx)
{
  static int PAGE_SIZE = 0;

  if (!PAGE_SIZE)
    PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

  assert(fct);

  /* allocate a fiber structre */
  melon_fiber * fiber = calloc(1, sizeof (melon_fiber));
  if (!fiber)
    return NULL;
  fiber->waited_event = kEventNone;

  /* allocate the stack */
  fiber->ctx.uc_link = NULL;
  fiber->ctx.uc_stack.ss_size = PAGE_SIZE;
  fiber->ctx.uc_stack.ss_sp = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED | MAP_GROWSDOWN | MAP_STACK,
                                   0, 0);
  if (!fiber->ctx.uc_stack.ss_sp)
  {
    free(fiber);
    return NULL;
  }

  /* initialize the context */
  makecontext(&fiber->ctx, (void (*)(void))fct, 1, ctx);

  pthread_mutex_lock(&g_melon.mutex);
  melon_list_push(g_melon.ready, fiber);
  pthread_mutex_unlock(&g_melon.mutex);
  return fiber;
}

void melon_fiber_join(struct melon_fiber * fiber)
{
  assert(fiber);
}

void melon_fiber_detach(struct melon_fiber * fiber)
{
  assert(fiber);
}
