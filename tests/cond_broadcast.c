#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

melon_mutex * mutex = NULL;
melon_cond * cond = NULL;
int val = 0;

static void * fct2(void * dummy)
{
  (void)dummy;
  melon_mutex_lock(mutex);
  printf("fct2: locked mutex\n");
  val = 42;
  melon_cond_broadcast(cond);
  printf("fct2: sent signal\n");
  melon_mutex_unlock(mutex);
  printf("fct2: released mutex\n");
  return NULL;
}

static void * fct1(void * dummy)
{
  (void)dummy;
  printf("fct1: xx\n");
  melon_mutex_lock(mutex);
  melon_fiber_createlight(NULL, fct2, NULL);
  printf("fct1: cond wait\n");
  melon_cond_wait(cond, mutex);
  printf("fct1: woke up, got val: %d\n", val);
  melon_mutex_unlock(mutex);
  return NULL;
}

int main(void)
{
  if (melon_init(0))
    return 1;
  melon_mutex_init(&mutex, NULL);
  melon_cond_init(&cond, NULL);

  for (int i = 0; i < 1000; ++i)
  {
    melon_fiber_createlight(NULL, fct1, NULL);
    melon_wait();
  }
  melon_mutex_destroy(mutex);
  melon_cond_destroy(cond);
  melon_deinit();
  return 0;
}
