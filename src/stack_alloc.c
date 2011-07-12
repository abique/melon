#include <assert.h>
#include <sys/mman.h>
#include "private.h"

#ifndef MAP_UNINITIALIZED
# define MAP_UNINITIALIZED 0x0
#endif

typedef struct list
{
  struct list * next;
} list;

static pthread_spinlock_t g_stack_list_lock;
static list *             g_stack_list = NULL;
static int                g_stack_list_nb = 0;

int melon_stack_init()
{
  assert(SIGSTKSZ % sysconf(_SC_PAGESIZE) == 0);
  g_stack_list = NULL;
  g_stack_list_nb = 0;
  if (pthread_spin_init(&g_stack_list_lock, PTHREAD_PROCESS_SHARED))
    return -1;
  return 0;
}

void melon_stack_deinit()
{
  pthread_spin_destroy(&g_stack_list_lock);
  list * item = g_stack_list;
  list * next;

  while (item)
  {
    next = item->next;
    munmap((void*)item, SIGSTKSZ);
    item = next;
  }
}

void * melon_stack_alloc()
{
  void * addr;
  list * item = g_stack_list;
  if (!item)
    goto alloc;

  pthread_spin_lock(&g_stack_list_lock);
  item = g_stack_list;
  if (!item)
  {
    pthread_spin_unlock(&g_stack_list_lock);
    alloc:
    addr = mmap(0, SIGSTKSZ, PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED |
                MAP_GROWSDOWN | MAP_STACK, 0, 0);
    if (addr == MAP_FAILED)
      return NULL;
    return addr;
  }

  g_stack_list = item->next;
  --g_stack_list_nb;
  assert(g_stack_list_nb >= 0);
  pthread_spin_unlock(&g_stack_list_lock);
  return (void*)item;
}

void melon_stack_free(void * addr)
{
  if (!addr)
    return;

  if (g_stack_list_nb > -1)
  {
    munmap(addr, SIGSTKSZ);
    return;
  }
  pthread_spin_lock(&g_stack_list_lock);
  assert(g_stack_list_nb >= 0);
  ++g_stack_list_nb;
  list * item = (list*)addr;
  item->next = g_stack_list;
  g_stack_list = item;
  pthread_spin_unlock(&g_stack_list_lock);
}

uint32_t melon_stack_size()
{
  return SIGSTKSZ;
}
