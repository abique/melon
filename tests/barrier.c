#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include "../src/melon.h"

melon_barrier * barrier = NULL;

static void * fct2(void * dummy)
{
  printf("%p: wait\n", dummy);
  melon_barrier_wait(barrier);
  printf("%p: end\n", dummy);
  return NULL;
}

static void * fct1(void * dummy)
{
  (void)dummy;

  printf("barrier_new()\n");
  melon_barrier_init(&barrier, NULL, 11);
  assert(barrier);

  for (int i = 1; i <= 10; ++i)
    melon_fiber_createlight(NULL, fct2, NULL + i);
  printf("slee(1)\n");
  melon_sleep(1);
  printf("wait()\n");
  melon_barrier_wait(barrier);
  printf("destroy()\n");
  melon_barrier_destroy(barrier);
  return NULL;
}

int main(void)
{
  if (melon_init(0))
    return 1;

  melon_fiber_createlight(NULL, fct1, NULL);

  melon_wait();
  melon_deinit();
  return 0;
}
