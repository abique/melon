#include <assert.h>
#include <stdlib.h>

#include "../src/private.h"

typedef struct item
{
  struct item * next;
  int value;
} item;

item * item_new(int value)
{
  item * item = malloc(sizeof (*item));
  assert(item);
  item->next = 0;
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

  /* check that we can pop a mal-formed list */
  list = items[0];
  list->next = NULL;
  melon_list_pop(list, tmp);
  assert(list == NULL);
  assert(tmp == items[0]);
  assert(tmp->next == NULL);

  /* check the empty case */
  melon_list_push(list, items[0]);
  assert(list == items[0]);
  assert(items[0]->next == items[0]);

  melon_list_pop(list, tmp);
  assert(!list);
  assert(tmp == items[0]);
  assert(!tmp->next);

  /* check 2 elements case */
  melon_list_push(list, items[0]);
  melon_list_push(list, items[1]);
  assert(list == items[1]);
  assert(items[0]->next == items[1]);
  assert(items[1]->next == items[0]);

  melon_list_pop(list, tmp);
  assert(!tmp->next);
  assert(tmp == items[0]);
  assert(list == items[1]);
  assert(items[1]->next == items[1]);
  melon_list_pop(list, tmp);
  assert(tmp == items[1]);
  assert(!list);

  /* push all then use pop to free */
  for (int i = 0; i < NB; ++i)
    melon_list_push(list, items[i]);

  /* check */
  assert(list == items[NB - 1]);
  for (int i = 0; i < NB; ++i)
    assert(items[i]->next == items[(i + 1) % NB]);

  /* pop and free */
  int nb = 0;
  while (1)
  {
    melon_list_pop(list, tmp);
    if (!tmp)
      break;
    free(tmp);
    ++nb;
  }
  assert(nb == NB);

  return 0;
}
