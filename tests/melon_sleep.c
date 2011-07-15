#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

static void timer(void * dummy)
{
  (void)dummy;
  for (int i = 0; i < 20; ++i)
    melon_usleep((rand() % 5) * 10000);
  melon_sleep(2);
}

MELON_MAIN(argc, argv)
{
  (void)argc;
  (void)argv;

  for (int i = 1; i <= 100; ++i)
  {
    if (melon_fiber_start(timer, NULL + i))
      break;
  }
  return 0;
}
