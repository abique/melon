#include <assert.h>
#include <stdlib.h>

#include "../src/private.h"

typedef struct item
{
  struct item * toto_next;
  struct item * toto_prev;
  int value;
} item;

item * item_new(int value)
{
  item * item = malloc(sizeof (*item));
  assert(item);
  item->toto_next = NULL;
  item->toto_prev = NULL;
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
  melon_dlist_push(list, items[0], toto_);
  assert(list == items[0]);
  assert(items[0]->toto_next == items[0]);
  assert(items[0]->toto_prev == items[0]);

  melon_dlist_pop(list, tmp, toto_);
  assert(!list);
  assert(tmp == items[0]);
  assert(!tmp->toto_next);
  assert(!tmp->toto_prev);

  /* check 2 elements case */
  melon_dlist_push(list, items[0], toto_);
  melon_dlist_push(list, items[1], toto_);
  assert(list == items[0]);
  assert(items[0]->toto_next == items[1]);
  assert(items[1]->toto_next == items[0]);
  assert(items[0]->toto_prev == items[1]);
  assert(items[1]->toto_prev == items[0]);

  melon_dlist_pop(list, tmp, toto_);
  assert(!tmp->toto_next);
  assert(!tmp->toto_prev);
  assert(tmp == items[0]);
  assert(list == items[1]);
  assert(items[1]->toto_next == items[1]);
  assert(items[1]->toto_prev == items[1]);
  melon_dlist_pop(list, tmp, toto_);
  assert(tmp == items[1]);
  assert(!tmp->toto_next);
  assert(!tmp->toto_prev);
  assert(!list);

  /* check insert */
  melon_dlist_insert(list, (item*)NULL, items[1], toto_);
  assert(list == items[1]);
  assert(items[1]->toto_prev == items[1]);
  assert(items[1]->toto_next == items[1]);

  melon_dlist_insert(list, items[1], items[2], toto_);
  assert(list == items[1]);
  assert(items[1]->toto_prev == items[2]);
  assert(items[1]->toto_next == items[2]);
  assert(items[2]->toto_prev == items[1]);
  assert(items[2]->toto_next == items[1]);

  melon_dlist_insert(list, (item*)NULL, items[0], toto_);
  assert(list == items[0]);
  assert(items[0]->toto_prev == items[2]);
  assert(items[0]->toto_next == items[1]);
  assert(items[1]->toto_prev == items[0]);
  assert(items[1]->toto_next == items[2]);
  assert(items[2]->toto_prev == items[1]);
  assert(items[2]->toto_next == items[0]);

  /* check unlink */
  melon_dlist_unlink(list, items[0], toto_);
  assert(list == items[1]);
  assert(!items[0]->toto_next);
  assert(!items[0]->toto_prev);
  assert(items[1]->toto_prev == items[2]);
  assert(items[1]->toto_next == items[2]);
  assert(items[2]->toto_prev == items[1]);
  assert(items[2]->toto_next == items[1]);

  melon_dlist_unlink(list, items[2], toto_);
  assert(list == items[1]);
  assert(items[1]->toto_prev == items[1]);
  assert(items[1]->toto_next == items[1]);
  assert(!items[2]->toto_next);
  assert(!items[2]->toto_prev);

  melon_dlist_unlink(list, items[1], toto_);
  assert(!list);
  assert(!items[1]->toto_next);
  assert(!items[1]->toto_prev);

  /* push all then use pop to free */
  for (int i = 0; i < NB; ++i)
    melon_dlist_push(list, items[i], toto_);

  /* check */
  assert(list == items[0]);
  for (int i = 0; i < NB; ++i)
  {
    assert(items[i]->toto_next == items[(i + 1) % NB]);
    assert(items[i]->toto_prev == items[(NB + i - 1) % NB]);
  }

  /* pop and free */
  int nb = 0;
  while (1)
  {
    melon_dlist_pop(list, tmp, toto_);
    if (!tmp)
      break;
    free(tmp);
    ++nb;
  }
  assert(nb == NB);

  return 0;
}
