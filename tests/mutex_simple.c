#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

melon_mutex * mutex = NULL;
int nb = 0;

static void fct(void * dummy)
{
  (void)dummy;
  for (int i = 0; i < 1000; ++i)
  {
    if (melon_mutex_trylock(mutex))
      if (melon_mutex_timedlock(mutex, melon_time() + 1000 * 50))
        melon_mutex_lock(mutex);
    ++nb;
    melon_mutex_unlock(mutex);
  }
  printf("%p finished\n", dummy);
}

int main(void)
{
  if (melon_init(0))
    return 1;

  mutex = melon_mutex_new(0);
  for (int i = 1; i <= 1000; ++i)
    if (melon_fiber_startlight(fct, NULL + i))
      break;

  melon_wait();
  melon_mutex_destroy(mutex);
  melon_deinit();
  return 0;
}
