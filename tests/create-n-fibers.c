#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "../src/melon.h"

static void dummy_fiber(void * dummy)
{
  (void)dummy;
  char buffer[1024];
  sprintf(buffer, "current thread: %ld\n", dummy - NULL);
  write(1, buffer, strlen(buffer));
}

int main(int argc, char ** argv)
{
  if (argc != 2)
    return 1;

  if (melon_init(0))
    return 1;

  int n = atoi(argv[1]);
  for (int i = 0; i < n; ++i)
  {
    printf("creating fiber %d\n", i);
    melon_fiber_start(dummy_fiber, NULL + i);
    printf("created fiber %d\n", i);
  }

  melon_wait();
  printf("wait finished\n");
  melon_deinit();
  fflush(0);
  return 0;
}
