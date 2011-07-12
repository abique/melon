#include <assert.h>

#include "../src/private.h"

# define NB 10000

int main()
{
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
  melon_stack_free(addrs[0]);
  melon_stack_free(addrs[1]);
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

  return 0;
}
