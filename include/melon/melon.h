#ifndef MELON_MELON_H
# define MELON_MELON_H

# include <stdint.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdio.h>

# if __unix__
#  include <sys/uio.h>
#  include <sys/socket.h>
#  include <sys/sendfile.h>
# endif

# if __WIN32__
#  include <windows.h>
#  include <winsock2.h>

struct iovec
{
  void * iov_base;
  size_t iov_len;
};
typedef int socklen_t;

# endif

# include "spinlock.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef int64_t                  melon_time_t;
  typedef struct melon_mutex       melon_mutex;
  typedef struct melon_mutexattr   melon_mutexattr;
  typedef struct melon_rwlock      melon_rwlock;
  typedef struct melon_rwlockattr  melon_rwlockattr;
  typedef struct melon_cond        melon_cond;
  typedef struct melon_condattr    melon_condattr;
  typedef struct melon_sem         melon_sem;
  typedef struct melon_semattr     melon_semattr;
  typedef struct melon_barrier     melon_barrier;
  typedef struct melon_barrierattr melon_barrierattr;
  typedef struct melon_fiber       melon_fiber;
  typedef struct melon_fiberattr   melon_fiberattr;

  /**
   * @name Initialisation
   * @{
   */
  /** initializes melon runtime
   * @return 0 on success, non zero else */
  int melon_init(uint32_t nb_threads);
  /** blocks until there is no more fibers left in melon */
  void melon_wait(void);
  /** deinitialize melon */
  void melon_deinit(void);
  /** adds threads to the thread pool */
  int melon_kthread_add(uint32_t nb);
  /** removes threads from the thread pool */
  int melon_kthread_release(uint32_t nb);
  /** @} */

  /**
   * @name Scheduler
   * @{
   */
  /** schedules next fibers and put the current one in the back
   * of the ready queue */
  void melon_yield(void);
  /** @} */

  /**
   * @name Fiber
   * @{
   */
  int melon_fiberattr_init(melon_fiberattr ** attr);
  void melon_fiberattr_destroy(melon_fiberattr * attr);
  void melon_fiberattr_setstacksize(melon_fiberattr * attr, uint32_t size);
  int melon_fiberattr_getstacksize(melon_fiberattr * attr);

  /** creates a new detached fiber
   * @return 0 on success */
  int melon_fiber_createlight(melon_fiberattr * attr, void * (*fct)(void *), void * ctx);

  /** if you don't need to join the fiber, prefer \ref melon_fiber_startlight
   * which is faster */
  int melon_fiber_create(melon_fiber ** fiber, melon_fiberattr * attr, void * (*fct)(void *), void * ctx);
  void melon_fiber_join(melon_fiber * fiber, void ** retval);
  int melon_fiber_tryjoin(melon_fiber * fiber, void ** retval);
  int melon_fiber_timedjoin(melon_fiber * fiber, void ** retval, melon_time_t timeout);
  void melon_fiber_detach(melon_fiber * fiber);
  melon_fiber * melon_fiber_self(void);
  const char * melon_fiber_name(const melon_fiber * fiber);
  void melon_fiber_setname(melon_fiber * fiber, const char * name);
  /** @} */

  /**
   * @name Time
   * @{
   */

# define MELON_NANOSECOND (1UL)
# define MELON_MICROSECOND (1000UL * MELON_NANOSECOND)
# define MELON_MILLISECOND (1000UL * MELON_MICROSECOND)
# define MELON_SECOND (1000UL * MELON_MILLISECOND)
# define MELON_MINUTE (60UL * MELON_SECOND)
# define MELON_HOUR (60UL * MELON_MINUTE)
# define MELON_DAY (24UL * MELON_HOUR)
# define MELON_WEEK (7UL * MELON_DAY)

  /** gets the time in nanoseconds */
  melon_time_t melon_time(void);
  unsigned int melon_sleep(uint32_t secs);
  int melon_usleep(uint64_t usecs);
  /** @} */

  /**
   * @name Mutex
   * @{
   */
  int melon_mutexattr_init(melon_mutexattr ** attr);
  void melon_mutexattr_destroy(melon_mutexattr * attr);
# define MELON_MUTEX_NORMAL 1
# define MELON_MUTEX_RECURSIVE 2
  void melon_mutexattr_settype(melon_mutexattr * attr, int type);
  int melon_mutexattr_gettype(melon_mutexattr * attr);

  int melon_mutex_init(melon_mutex ** mutex, melon_mutexattr * attr);
  void melon_mutex_destroy(melon_mutex * mutex);
  void melon_mutex_lock(melon_mutex * mutex);
  void melon_mutex_unlock(melon_mutex * mutex);
  int melon_mutex_trylock(melon_mutex * mutex);
  int melon_mutex_timedlock(melon_mutex * mutex, uint64_t timeout);
  /** @} */

  /**
   * @name Read/Write lock
   * @{
   */
  int melon_rwlockattr_init(melon_rwlockattr ** attr);
  void melon_rwlockattr_destroy(melon_rwlockattr * attr);

  int melon_rwlock_init(melon_rwlock ** rwlock, melon_rwlockattr * attr);
  void melon_rwlock_destroy(melon_rwlock * rwlock);
  void melon_rwlock_rdlock(melon_rwlock * rwlock);
  int melon_rwlock_tryrdlock(melon_rwlock * rwlock);
  int melon_rwlock_timedrdlock(melon_rwlock * rwlock, uint64_t timeout);
  void melon_rwlock_wrlock(melon_rwlock * rwlock);
  int melon_rwlock_trywrlock(melon_rwlock * rwlock);
  int melon_rwlock_timedwrlock(melon_rwlock * rwlock, uint64_t timeout);
  void melon_rwlock_unlock(melon_rwlock * rwlock);
  /** @} */

  /**
   * @name Condition
   * @{
   */
  int melon_condattr_init(melon_condattr ** attr);
  void melon_condattr_destroy(melon_condattr * attr);

  int melon_cond_init(melon_cond ** cond, melon_condattr * attr);
  void melon_cond_destroy(melon_cond * condition);
  void melon_cond_wait(melon_cond * condition, melon_mutex * mutex);
  int melon_cond_timedwait(melon_cond * condition, melon_mutex * mutex, uint64_t timeout);
  /** @return the number of woke up fibers */
  int melon_cond_signal(melon_cond * condition);
  /** if nb == 1, then equivalent to melon_cond_signal, if nb == 0
   * then equivalent to melon_cond_broadcast
   * @return the number of woke up fibers */
  int melon_cond_signalnb(melon_cond * condition, uint32_t nb);
  /** @return the number of woke up fibers */
  int melon_cond_broadcast(melon_cond * condition);
  /** @} */

  /**
   * @name Semaphore
   * @{
   */
  int melon_semattr_init(melon_semattr ** attr);
  void melon_semattr_destroy(melon_semattr * attr);

  int melon_sem_init(melon_sem ** sem, melon_semattr * attr, uint32_t nb);
  void melon_sem_destroy(melon_sem * sem);
  void melon_sem_acquire(melon_sem * sem, uint32_t nb);
  int melon_sem_tryacquire(melon_sem * sem, uint32_t nb);
  int melon_sem_timedacquire(melon_sem * sem, uint32_t nb, melon_time_t timeout);
  void melon_sem_release(melon_sem * sem, uint32_t nb);
  /** @} */

  /**
   * @name Barrier
   * @{
   */
  int melon_barrierattr_init(melon_barrierattr ** attr);
  void melon_barrierattr_destroy(melon_barrierattr * attr);

  int melon_barrier_init(melon_barrier ** barrier, melon_barrierattr * attr, uint32_t nb);
  void melon_barrier_destroy(melon_barrier * barrier);
  void melon_barrier_add(melon_barrier * barrier, uint32_t nb);
  void melon_barrier_release(melon_barrier * barrier, uint32_t nb);
  void melon_barrier_wait(melon_barrier * barrier);
  /** @} */

  /**
   * @name Input/Output
   * @{
   */
  /**
   * @fn melon_close
   * @brief closes a file descriptor
   * When called, this function wake up fibers waiting for events on this
   * file descriptor and then closes it. Woked up fibers will receive ECANCELED
   * error on their I/O.
   * @param fildes the file descriptor to close
   */
  int melon_close(int fildes);
  /** cancel pending io on fildes */
  int melon_cancelio(int fildes);
  int64_t melon_write(int fildes, const void * data, uint64_t nbyte, melon_time_t timeout);
  int64_t melon_pwrite(int fildes, const void * data, uint64_t nbyte, int64_t offset, melon_time_t timeout);
  int64_t melon_writev(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout);
  int64_t melon_pwritev(int fildes, const struct iovec *iov, int iovcnt, int64_t offset, melon_time_t timeout);

  int64_t melon_read(int fildes, void * data, uint64_t nbyte, melon_time_t timeout);
  int64_t melon_pread(int fildes, void * data, uint64_t nbyte, int64_t offset, melon_time_t timeout);
  int64_t melon_readv(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout);
  int64_t melon_preadv(int fildes, const struct iovec *iov, int iovcnt, int64_t offset, melon_time_t timeout);

  int melon_connect(int socket, const struct sockaddr *address, socklen_t address_len, melon_time_t timeout);
  int melon_accept(int socket, struct sockaddr * address, socklen_t * address_len, melon_time_t timeout);
  int64_t melon_sendto(int socket, const void *message, uint64_t length,
                       int flags, const struct sockaddr *dest_addr,
                       socklen_t dest_len, melon_time_t timeout);
  int64_t melon_recvfrom(int socket, void *buffer, uint64_t length,
                         int flags, struct sockaddr * address,
                         socklen_t * address_len, melon_time_t timeout);
  int64_t melon_recvmsg(int socket, struct msghdr *message, int flags, melon_time_t timeout);
  int64_t melon_sendmsg(int socket, const struct msghdr *message, int flags, melon_time_t timeout);

  int64_t melon_sendfile(int out_fd, int in_fd, int64_t *offset, uint64_t count, melon_time_t timeout);

#  ifdef _GNU_SOURCE
  int64_t melon_splice(int fd_in, int64_t *off_in, int fd_out,
                       int64_t *off_out, uint64_t len, unsigned int flags, melon_time_t timeout);
  int64_t melon_tee(int fd_in, int fd_out, uint64_t len, unsigned int flags, melon_time_t timeout);
  int64_t melon_vmsplice(int fd, const struct iovec *iov,
                         unsigned long nr_segs, unsigned int flags, melon_time_t timeout);
#  endif

  FILE * melon_fopen(const char * path, const char * mode, const melon_time_t * timeout);
  FILE * melon_fdopen(int fd, const char * mode, const melon_time_t * timeout);

  /** @} */


# define MELON_MAIN(Argc, Argv)                                 \
  typedef struct                                                \
  {                                                             \
    int     argc;                                               \
    char ** argv;                                               \
    int     retval;                                             \
  }  __melon_main_data;                                         \
                                                                \
  static int __melon_main(int argc, char ** argv);              \
  static void * __main_wrapper(void * data)                     \
  {                                                             \
    __melon_main_data * md = (__melon_main_data*)data;          \
    md->retval = __melon_main(md->argc, md->argv);              \
    return NULL;                                                \
  }                                                             \
                                                                \
  int main(int argc, char ** argv)                              \
  {                                                             \
    if (melon_init(0))                                          \
    {                                                           \
      fprintf(stderr, "failed to initialize melon.\n");         \
      return 1;                                                 \
    }                                                           \
                                                                \
    __melon_main_data data = { argc, argv, 0 };;                \
    if (melon_fiber_createlight(NULL, __main_wrapper, &data))   \
    {                                                           \
      fprintf(stderr, "failed to create the first fiber.\n");   \
      melon_deinit();                                           \
      return 1;                                                 \
    }                                                           \
    melon_wait();                                               \
    melon_deinit();                                             \
    return data.retval;                                         \
  }                                                             \
                                                                \
  static int __melon_main(int Argc, char ** Argv)

# ifdef MELON_OVERRIDE_POSIX

#  ifdef sleep
#   undef sleep
#  endif
#  define sleep(Args...) melon_sleep(Args)

#  ifdef usleep
#   undef usleep
#  endif
#  define usleep(Args...) melon_usleep(Args)

#  ifdef close
#   undef close
#  endif
#  define close(Args...) melon_close(Args)

/*

  for func in \
  write pwrite writev prwitev \
  read pread readv preadv \
  connect accept \
  sendto sendmsg recvfrom recvmsg \
  splice
  do
  cat <<EOF
  #  ifdef $func
  #   undef $func
  #  endif
  #  define $func(Args...) melon_$func(Args, 0)

  EOF
  done

*/

#  ifdef write
#   undef write
#  endif
#  define write(Args...) melon_write(Args, 0)

#  ifdef pwrite
#   undef pwrite
#  endif
#  define pwrite(Args...) melon_pwrite(Args, 0)

#  ifdef writev
#   undef writev
#  endif
#  define writev(Args...) melon_writev(Args, 0)

#  ifdef prwitev
#   undef prwitev
#  endif
#  define prwitev(Args...) melon_prwitev(Args, 0)

#  ifdef read
#   undef read
#  endif
#  define read(Args...) melon_read(Args, 0)

#  ifdef pread
#   undef pread
#  endif
#  define pread(Args...) melon_pread(Args, 0)

#  ifdef readv
#   undef readv
#  endif
#  define readv(Args...) melon_readv(Args, 0)

#  ifdef preadv
#   undef preadv
#  endif
#  define preadv(Args...) melon_preadv(Args, 0)

#  ifdef connect
#   undef connect
#  endif
#  define connect(Args...) melon_connect(Args, 0)

#  ifdef accept
#   undef accept
#  endif
#  define accept(Args...) melon_accept(Args, 0)

#  ifdef sendto
#   undef sendto
#  endif
#  define sendto(Args...) melon_sendto(Args, 0)

#  ifdef sendmsg
#   undef sendmsg
#  endif
#  define sendmsg(Args...) melon_sendmsg(Args, 0)

#  ifdef recvfrom
#   undef recvfrom
#  endif
#  define recvfrom(Args...) melon_recvfrom(Args, 0)

#  ifdef recvmsg
#   undef recvmsg
#  endif
#  define recvmsg(Args...) melon_recvmsg(Args, 0)

#  ifdef splice
#   undef splice
#  endif
#  define splice(Args...) melon_splice(Args, 0)

# endif /* MELON_OVERRIDE_POSIX */

# ifdef __cplusplus
}
# endif

#endif /* !MELON_MELON_H */
