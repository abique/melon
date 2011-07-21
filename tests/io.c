#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "../src/melon.h"

int fd[2];

#define STR "Hello World!\n"

static void fct2(void * dummy)
{
  (void)dummy;

  char buffer[1024];

  printf("fct2: read\n");
  ssize_t nb = melon_read(fd[0], buffer, sizeof (buffer), 42);
  assert(nb == -1 && errno == -ETIMEDOUT);
  printf("fct2: read\n");
  nb = melon_read(fd[0], buffer, sizeof (buffer), 0);
  assert(nb > 0);
  printf("fct2: write\n");
  melon_write(STDOUT_FILENO, buffer, nb, 0);
  printf("fct2: close\n");
  close(fd[0]);
  printf("fct2: end\n");
}

static void fct1(void * dummy)
{
  (void)dummy;
  printf("fct1: sleep\n");
  melon_sleep(1);
  printf("fct1: write\n");
  melon_write(fd[1], STR, sizeof (STR), 0);
  printf("fct1: close\n");
  close(fd[1]);
  printf("fct1: end\n");
}

int main(void)
{
  if (melon_init(0))
    return 1;

  if (pipe(fd))
    return 1;

  melon_fiber_startlight(fct1, NULL);
  melon_fiber_startlight(fct2, NULL);

  melon_wait();
  melon_deinit();
  return 0;
}
