#ifndef MELON_PTHREAD2MELON_H
# define MELON_PTHREAD2MELON_H

# include "melon.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef melon_mutex *       pthread_mutex_t;
  typedef melon_mutexattr *   pthread_mutexattr_t;
  typedef melon_rwlock *      pthread_rwlock_t;
  typedef melon_rwlockattr *  pthread_rwlockattr_t;
  typedef melon_cond *        pthread_cond_t;
  typedef melon_condattr *    pthread_condattr_t;
  typedef melon_sem *         pthread_sem_t;
  typedef melon_semattr *     pthread_semattr_t;
  typedef melon_barrier *     pthread_barrier_t;
  typedef melon_barrierattr * pthread_barrierattr_t;
  typedef melon_fiber *       pthread_t;
  typedef melon_fiberattr *   pthread_attr_t;

  /*

for func in $(man -k pthread | grep pthread_ | cut -f 1 -d ' ' | sort | uniq)
do
cat <<EOF
# ifdef $func
#  undef $func
# endif
# define $func(Args...) melon_${func/pthread_/}(Args)

EOF
done

  */


# ifdef __cplusplus
}
# endif

#endif /* !MELON_PTHREAD2MELON_H */
