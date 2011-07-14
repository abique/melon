#ifndef DLIST_H
# define DLIST_H

// circular linked list
# define melon_dlist_push(List, Item, Prefix)           \
  do {                                                  \
    if (!(List))                                        \
    {                                                   \
      (List) = (Item);                                  \
      (Item)->Prefix##_next = (Item);                   \
      (Item)->Prefix##_Prefix##_prev = (Item);          \
    }                                                   \
    else                                                \
    {                                                   \
      (Item)->Prefix##_next = (List);                   \
      (Item)->Prefix##_prev = (List)->Prefix##_prev;    \
      (Item)->Prefix##_next->Prefix##_prev = (Item);    \
      (Item)->Prefix##_prev->Prefix##_next = (Item);    \
    }                                                   \
  } while (0)

# define melon_dlist_unlink(List, Item, Prefix)                         \
  do {                                                                  \
    /* just 1 element */                                                \
    if ((List) == (List)->Prefix##_next)                                \
    {                                                                   \
      (Item)->Prefix##_next = NULL;                                     \
      (Item)->Prefix##_prev = NULL;                                     \
      (List) = NULL;                                                    \
    }                                                                   \
    /* many elements */                                                 \
    else                                                                \
    {                                                                   \
      (Item)->Prefix##_next->Prefix##_prev = (Item)->Prefix##_prev;     \
      (Item)->Prefix##_prev->Prefix##_next = (Item)->Prefix##_next;     \
      (Item)->Prefix##_next = NULL;                                     \
      (Item)->Prefix##_prev = NULL;                                     \
    }                                                                   \
  } while (0)


# define melon_dlist_pop(List, Item, Prefix)    \
  do {                                          \
    /* empty list */                            \
    if (!(List))                                \
      (Item) = NULL;                            \
    else                                        \
    {                                           \
      (Item) = (List)->Prefix##_prev;           \
      melon_dlist_unlink(List, (Item), Prefix); \
    }                                           \
  } while (0)


#endif /* !DLIST_H */
