#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "../src/melon.h"

#define STR "Hello World!\n"

static void * fct1(void * dummy)
{
  (void)dummy;

  int64_t ret = 0;
  ret = melon_write(42, STR, sizeof (STR), 0);
  printf("write: %ld, %s\n", ret, strerror(errno));

  ret = melon_read(42, STR, sizeof (STR), 0);
  printf("read: %ld, %s\n", ret, strerror(errno));

  printf("cancelio: %d, %s\n", melon_cancelio(42), strerror(errno));
  printf("close: %d, %s\n", melon_close(42), strerror(errno));

  return NULL;
}

int main(void)
{
  if (melon_init(0))
    return 1;

  melon_fiber_createlight(NULL, fct1, NULL);

  puts("waiting");
  melon_wait();
  puts("wait finished");
  melon_deinit();
  return 0;
}
