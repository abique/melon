#ifndef LIST_H
# define LIST_H

// circular linked list
# define melon_list_push(List, Item, Member)    \
  do {                                          \
    if (!(List))                                \
    {                                           \
      (List) = (Item);                          \
      (Item)->Member = (Item);                  \
    }                                           \
    else                                        \
    {                                           \
      (Item)->Member = (List)->Member;          \
      (List)->Member = (Item);                  \
      (List) = (Item);                          \
    }                                           \
  } while (0)

# define melon_list_pop(List, Item, Member)     \
  do {                                          \
    /* empty list */                            \
    if (!(List))                                \
      (Item) = NULL;                            \
    /* just 1 element */                        \
    else if ((List) == (List)->Member ||        \
             (List)->Member == NULL)            \
    {                                           \
      (Item) = (List);                          \
      (Item)->Member = NULL;                    \
      (List) = NULL;                            \
    }                                           \
    /* many elements */                         \
    else                                        \
    {                                           \
      (Item) = (List)->Member;                  \
      (List)->Member = (Item)->Member;          \
      (Item)->Member = NULL;                    \
    }                                           \
  } while (0)

#endif /* !LIST_H */
