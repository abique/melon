#include <assert.h>

#include "../src/private.h"

# define NB 1000

void * stack_check(void * ctx)
{
  (void)ctx;
  void *addrs[NB];

  for (int x = 0; x < 12; ++x)
  {
    for (int i = 0; i < NB; ++i)
      addrs[i] = melon_stack_alloc();

    for (int i = 0; i < NB; ++i)
      for (int j = i + 1; j < NB; ++j)
        assert(addrs[i] != addrs[j]);

    for (int i = 0; i < NB; ++i)
      melon_stack_free(addrs[i]);

    for (int i = 0; i < NB; ++i)
      addrs[i] = melon_stack_alloc();

    for (int i = 0; i < NB; ++i)
      melon_stack_free(addrs[NB - 1 - i]);
  }

  return NULL;
}

int main(int argc, char ** argv)
{
  (void)argc;
  (void)argv;

  void *addrs[NB];

  melon_stack_init();
  addrs[0] = melon_stack_alloc();
  melon_stack_free(addrs[0]);
  melon_stack_deinit();

  melon_stack_init();
  addrs[0] = melon_stack_alloc();
  melon_stack_free(addrs[0]);

  addrs[0] = melon_stack_alloc();
  addrs[1] = melon_stack_alloc();
  melon_stack_free(addrs[0]);
  melon_stack_free(addrs[1]);

  addrs[0] = melon_stack_alloc();
  addrs[1] = melon_stack_alloc();
  melon_stack_free(addrs[1]);
  melon_stack_free(addrs[0]);
  melon_stack_deinit();

  melon_stack_init();
  for (int i = 0; i < NB; ++i)
    addrs[i] = melon_stack_alloc();

  for (int i = 0; i < NB; ++i)
    for (int j = i + 1; j < NB; ++j)
      assert(addrs[i] != addrs[j]);

  for (int i = 0; i < NB; ++i)
    melon_stack_free(addrs[i]);

  for (int i = 0; i < NB; ++i)
    addrs[i] = melon_stack_alloc();

  for (int i = 0; i < NB; ++i)
    melon_stack_free(addrs[NB - 1 - i]);
  melon_stack_deinit();

  for (int v = 0; v < 4; ++v)
  {
    melon_stack_init();
    pthread_t threads[8];
    for (int i = 0; i < 8; ++i)
      pthread_create(threads + i, NULL, stack_check, NULL);
    for (int i = 0; i < 8; ++i)
      pthread_join(threads[i], NULL);
    melon_stack_deinit();
  }

  return 0;
}
