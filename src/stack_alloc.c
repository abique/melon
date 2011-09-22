#include <assert.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "private.h"

#ifndef MAP_UNINITIALIZED
# define MAP_UNINITIALIZED 0x0
#endif

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS 0x0
#endif

#ifndef MAP_ANON
# define MAP_ANON 0x0
#endif

#ifndef SIGSTKSZ
# define SIGSTKSZ (64 * 1024)
#endif

#define MELON_STACK_SIZE (SIGSTKSZ >= 16 * 1024 ? SIGSTKSZ : 16 * 1024)

typedef struct list
{
  struct list * next;
} list;

static melon_spinlock g_stack_list_lock;
static list *         g_stack_list    = NULL;
static int            g_stack_list_nb = 0;

int melon_stack_init()
{
  g_stack_list = NULL;
  g_stack_list_nb = 0;
  melon_spin_init(&g_stack_list_lock);
  return 0;
}

void melon_stack_deinit()
{
  list * item;

  melon_spin_lock(&g_stack_list_lock);
  while (1)
  {
    melon_list_pop(g_stack_list, item, next);
    if (!item)
      break;
    munmap((void*)item, MELON_STACK_SIZE);
  }
  melon_spin_unlock(&g_stack_list_lock);
}

void * melon_stack_alloc()
{
  melon_spin_lock(&g_stack_list_lock);
  if (!g_stack_list)
  {
    melon_spin_unlock(&g_stack_list_lock);
    void * addr = mmap(NULL /* addr */, MELON_STACK_SIZE /* size */,
                       PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_ANON |
		       MAP_UNINITIALIZED,
                       -1 /* fd */, 0 /* offset */);
    if (addr == MAP_FAILED)
      return NULL;
    return addr;
  }

  list * item;
  melon_list_pop(g_stack_list, item, next);
  --g_stack_list_nb;
  melon_spin_unlock(&g_stack_list_lock);
  return (void*)item;
}

void melon_stack_free(void * addr)
{
  assert(addr);

  if (g_stack_list_nb > 100)
  {
    munmap(addr, MELON_STACK_SIZE);
    return;
  }

  melon_spin_lock(&g_stack_list_lock);
  ++g_stack_list_nb;
  list * item = (list*)addr;
  melon_list_push(g_stack_list, item, next);
  melon_spin_unlock(&g_stack_list_lock);
}

uint32_t melon_stack_size()
{
  return MELON_STACK_SIZE;
}
