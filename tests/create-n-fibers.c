#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

static void dummy_fiber(void * dummy)
{
  (void)dummy;
}

int nb_created = 0;

int main(int argc, char ** argv)
{
  (void)argc;
  (void)argv;

  if (argc != 2)
    return 1;

  if (melon_init(0))
    return 1;

  int n = atoi(argv[1]);
  for (nb_created = 0; nb_created < n; ++nb_created)
  {
    struct melon_fiber * fiber = melon_fiber_start(dummy_fiber, NULL + nb_created);
    if (!fiber)
      break;
    melon_fiber_detach(fiber);
  }

  printf("waiting\n");
  melon_wait();
  printf("wait finished\n");
  melon_deinit();
  fflush(0);
  return 0;
}
