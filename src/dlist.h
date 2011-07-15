#ifndef DLIST_H
# define DLIST_H

// circular linked list
# define melon_dlist_push(List, Item, Prefix)           \
  do {                                                  \
    if (!(List))                                        \
    {                                                   \
      (List) = (Item);                                  \
      (Item)->Prefix##next = (Item);                    \
      (Item)->Prefix##prev = (Item);                    \
    }                                                   \
    else                                                \
    {                                                   \
      (Item)->Prefix##next = (List);                    \
      (Item)->Prefix##prev = (List)->Prefix##prev;      \
      (Item)->Prefix##next->Prefix##prev = (Item);      \
      (Item)->Prefix##prev->Prefix##next = (Item);      \
    }                                                   \
  } while (0)

# define melon_dlist_insert(List, Prev, Item, Prefix)   \
  do {                                                  \
    if (!(List))                                        \
    {                                                   \
      (List) = (Item);                                  \
      (Item)->Prefix##next = (Item);                    \
      (Item)->Prefix##prev = (Item);                    \
    }                                                   \
    else if (!Prev) /* push_front */                    \
    {                                                   \
      (Item)->Prefix##next = (List);                    \
      (Item)->Prefix##prev = (List)->Prefix##prev;      \
      (Item)->Prefix##next->Prefix##prev = (Item);      \
      (Item)->Prefix##prev->Prefix##next = (Item);      \
      (List) = (Item);                                  \
    }                                                   \
    else                                                \
    {                                                   \
      (Item)->Prefix##next = (Prev)->Prefix##next;      \
      (Item)->Prefix##prev = (Prev);                    \
      (Item)->Prefix##next->Prefix##prev = (Item);      \
      (Item)->Prefix##prev->Prefix##next = (Item);      \
    }                                                   \
  } while (0)

# define melon_dlist_unlink(List, Item, Prefix)                         \
  do {                                                                  \
    /* just 1 element */                                                \
    if ((List) == (List)->Prefix##next)                                 \
    {                                                                   \
      (Item)->Prefix##next = NULL;                                      \
      (Item)->Prefix##prev = NULL;                                      \
      (List) = NULL;                                                    \
    }                                                                   \
    /* many elements */                                                 \
    else                                                                \
    {                                                                   \
      if ((List) == (Item))                                             \
        (List) = (Item)->Prefix##next;                                  \
      (Item)->Prefix##next->Prefix##prev = (Item)->Prefix##prev;        \
      (Item)->Prefix##prev->Prefix##next = (Item)->Prefix##next;        \
      (Item)->Prefix##next = NULL;                                      \
      (Item)->Prefix##prev = NULL;                                      \
                                                                        \
    }                                                                   \
  } while (0)


# define melon_dlist_pop(List, Item, Prefix)            \
  do {                                                  \
    (Item) = (List);                                    \
    if ((Item))                                         \
      melon_dlist_unlink((List), (Item), Prefix);       \
  } while (0)


#endif /* !DLIST_H */
