#include <assert.h>
#include <stdlib.h>

#include "../src/private.h"

typedef struct item
{
  struct item * next;
  struct item * prev;
  int value;
} item;

item * item_new(int value)
{
  item * item = malloc(sizeof (*item));
  assert(item);
  item->next = NULL;
  item->prev = NULL;
  item->value = value;
  return item;
}

#define NB 16

int main()
{
  item * list = NULL;
  item * items[NB];
  item * tmp = NULL;

  for (int i = 0; i < NB; ++i)
    items[i] = item_new(i);

  /* check the empty case */
  melon_dlist_push(list, items[0], );
  assert(list == items[0]);
  assert(items[0]->next == items[0]);
  assert(items[0]->prev == items[0]);

  melon_dlist_pop(list, tmp, );
  assert(!list);
  assert(tmp == items[0]);
  assert(!tmp->next);
  assert(!tmp->prev);

  /* check 2 elements case */
  melon_dlist_push(list, items[0], );
  melon_dlist_push(list, items[1], );
  assert(list == items[0]);
  assert(items[0]->next == items[1]);
  assert(items[1]->next == items[0]);
  assert(items[0]->prev == items[1]);
  assert(items[1]->prev == items[0]);

  melon_dlist_pop(list, tmp, );
  assert(!tmp->next);
  assert(!tmp->prev);
  assert(tmp == items[0]);
  assert(list == items[1]);
  assert(items[1]->next == items[1]);
  assert(items[1]->prev == items[1]);
  melon_dlist_pop(list, tmp, );
  assert(tmp == items[1]);
  assert(!tmp->next);
  assert(!tmp->prev);
  assert(!list);

  /* push all then use pop to free */
  for (int i = 0; i < NB; ++i)
    melon_dlist_push(list, items[i], );

  /* check */
  assert(list == items[0]);
  for (int i = 0; i < NB; ++i)
  {
    assert(items[i]->next == items[(i + 1) % NB]);
    assert(items[i]->prev == items[(NB + i - 1) % NB]);
  }

  /* pop and free */
  int nb = 0;
  while (1)
  {
    melon_dlist_pop(list, tmp, );
    if (!tmp)
      break;
    free(tmp);
    ++nb;
  }
  assert(nb == NB);

  return 0;
}
