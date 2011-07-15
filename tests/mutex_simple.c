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
    melon_mutex_lock(mutex);
    ++nb;
    melon_mutex_unlock(mutex);
  }
  printf("%p finished\n", dummy);
}

MELON_MAIN(argc, argv)
{
  (void)argc;
  (void)argv;

  mutex = melon_mutex_new();
  for (int i = 1; i <= 100; ++i)
    if (melon_fiber_start(fct, NULL + i))
      break;
  melon_wait();
  melon_mutex_destroy(mutex);
  return 0;
}
