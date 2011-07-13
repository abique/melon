#ifndef SPINLOCK_H
# define SPINLOCK_H

typedef int melon_spinlock;

static inline void melon_spin_init(melon_spinlock * lock)
{
  *lock = 0;
}

static inline void melon_spin_lock(melon_spinlock * lock)
{
  while (!__sync_bool_compare_and_swap(lock, 0, 1))
    continue;
}

static inline void melon_spin_unlock(melon_spinlock * lock)
{
  *lock = 0;
}

#endif /* !SPINLOCK_H */
