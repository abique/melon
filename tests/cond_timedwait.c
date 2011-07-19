#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "../src/melon.h"

melon_mutex * mutex = NULL;
melon_cond * cond = NULL;
int val = 0;

static void fct2(void * dummy)
{
  (void)dummy;
  melon_mutex_lock(mutex);
  printf("fct2: locked mutex\n");
  val = 42;
  melon_cond_broadcast(cond);
  printf("fct2: sent signal, now cond_timedwait\n");
  int ret = melon_cond_timedwait(cond, mutex, 42);
  assert(ret == -ETIMEDOUT);
  assert(errno == ETIMEDOUT);
  printf("fct2: woke up\n");
  melon_mutex_unlock(mutex);
  printf("fct2: released mutex\n");
}

static void fct1(void * dummy)
{
  (void)dummy;
  printf("fct1: xx\n");
  melon_mutex_lock(mutex);
  melon_fiber_start(fct2, NULL);
  printf("fct1: cond timed wait\n");
  int ret = melon_cond_timedwait(cond, mutex, melon_time() + 1000 * 1000 * 100);
  printf("fct1: woke up, got val: %d\n", val);
  assert(val == 42);
  assert(!ret);
  melon_mutex_unlock(mutex);
  printf("fct1: released mutex\n");
}

int main(void)
{
  if (melon_init(0))
    return 1;
  mutex = melon_mutex_new();
  cond = melon_cond_new();
  melon_fiber_start(fct1, NULL);
  melon_wait();
  melon_mutex_destroy(mutex);
  melon_cond_destroy(cond);
  melon_deinit();
  return 0;
}
