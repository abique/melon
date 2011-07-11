#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

static void dummy_fiber(void * dummy)
{
  (void)dummy;
  /* char buffer[1024]; */
  /* sprintf(buffer, "dummy: %ld\n", dummy - NULL); */
  /* write(STDOUT_FILENO, buffer, strlen(buffer)); */
}

int main(int argc, char ** argv)
{
  if (argc != 2)
    return 1;

  if (melon_init(0))
    return 1;

  int n = atoi(argv[1]);
  for (int i = 1; i <= n; ++i)
  {
    struct melon_fiber * fiber = melon_fiber_start(dummy_fiber, NULL + i);
    melon_fiber_detach(fiber);
  }

  printf("waiting\n");
  melon_wait();
  printf("wait finished\n");
  melon_deinit();
  fflush(0);
  return 0;
}
