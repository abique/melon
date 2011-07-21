#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

static void * dummy_fiber(void * dummy)
{
  (void)dummy;
  return NULL;
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
    if (melon_fiber_startlight(dummy_fiber, NULL + nb_created))
      break;
  }

  printf("waiting\n");
  melon_wait();
  printf("wait finished\n");
  melon_deinit();
  return 0;
}
