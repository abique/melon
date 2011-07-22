#ifndef MELON_SPINLOCK_H
# define MELON_SPINLOCK_H

# ifdef __cplusplus
extern "C" {
# endif

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

static inline void melon_spin_destroy(melon_spinlock * lock)
{
  (void)lock;
}

# ifdef __cplusplus
}
# endif

#endif /* !MELON_SPINLOCK_H */
