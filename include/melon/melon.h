#ifndef MELON_MELON_H
# define MELON_MELON_H

# include <stdint.h>
# include <unistd.h>
# include <sys/uio.h>
# include <sys/socket.h>
# include <sys/sendfile.h>
# include <fcntl.h>

# include "spinlock.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef int64_t              melon_time_t;
  typedef struct melon_mutex   melon_mutex;
  typedef struct melon_rwlock  melon_rwlock;
  typedef struct melon_cond    melon_cond;
  typedef struct melon_sem     melon_sem;
  typedef struct melon_barrier melon_barrier;
  typedef struct melon_fiber   melon_fiber;

  /**
   * Melon runtime
   * @{
   */
  /** initializes melon runtime
   * @return 0 on success, non zero else */
  int melon_init(uint16_t nb_threads);
  /** blocks until there is no more fibers left in melon */
  void melon_wait(void);
  void melon_deinit(void);

  /** creates a new detached fiber
   * @return 0 on success */
  int melon_fiber_startlight(void * (*fct)(void *), void * ctx);

  /** if you don't need to join the fiber, prefer \ref melon_fiber_startlight
   * which is faster */
  melon_fiber * melon_fiber_start(void * (*fct)(void *), void * ctx);
  void melon_fiber_join(melon_fiber * fiber, void ** retval);
  int melon_fiber_tryjoin(melon_fiber * fiber, void ** retval);
  int melon_fiber_timedjoin(melon_fiber * fiber, void ** retval, melon_time_t timeout);
  void melon_fiber_detach(melon_fiber * fiber);
  melon_fiber * melon_fiber_self(void);
  const char * melon_fiber_name(const melon_fiber * fiber);
  void melon_fiber_setname(melon_fiber * fiber, const char * name);

  /** schedules next fibers and put the current one in the back
   * of the ready queue */
  void melon_yield(void);

  /** gets the time in nanoseconds */
  melon_time_t melon_time(void);
  /** @} */

  /**
   * Synchronisation functions
   * @{
   */
  melon_mutex * melon_mutex_new(int is_recursive);
  void melon_mutex_destroy(melon_mutex * mutex);
  void melon_mutex_lock(melon_mutex * mutex);
  void melon_mutex_unlock(melon_mutex * mutex);
  int melon_mutex_trylock(melon_mutex * mutex);
  int melon_mutex_timedlock(melon_mutex * mutex, uint64_t timeout);

  melon_rwlock * melon_rwlock_new(void);
  void melon_rwlock_destroy(melon_rwlock * rwlock);
  void melon_rwlock_rdlock(melon_rwlock * rwlock);
  int melon_rwlock_tryrdlock(melon_rwlock * rwlock);
  int melon_rwlock_timedrdlock(melon_rwlock * rwlock, uint64_t timeout);
  void melon_rwlock_wrlock(melon_rwlock * rwlock);
  int melon_rwlock_trywrlock(melon_rwlock * rwlock);
  int melon_rwlock_timedwrlock(melon_rwlock * rwlock, uint64_t timeout);
  void melon_rwlock_unlock(melon_rwlock * rwlock);

  melon_cond * melon_cond_new(void);
  void melon_cond_destroy(melon_cond * condition);
  void melon_cond_wait(melon_cond * condition, melon_mutex * mutex);
  int melon_cond_timedwait(melon_cond * condition, melon_mutex * mutex, uint64_t timeout);
  void melon_cond_signal(melon_cond * condition);
  void melon_cond_broadcast(melon_cond * condition);

  melon_sem * melon_sem_new(int nb);
  void melon_sem_destroy(melon_sem * sem);
  void melon_sem_acquire(melon_sem * sem, int nb);
  int melon_sem_tryacquire(melon_sem * sem, int nb);
  int melon_sem_timedacquire(melon_sem * sem, int nb, melon_time_t timeout);
  void melon_sem_release(melon_sem * sem, int nb);

  melon_barrier * melon_barrier_new(uint16_t nb);
  void melon_barrier_destroy(melon_barrier * barrier);
  void melon_barrier_add(melon_barrier * barrier, uint16_t nb);
  void melon_barrier_release(melon_barrier * barrier, uint16_t nb);
  void melon_barrier_wait(melon_barrier * barrier);
  /** @} */

  /**
   * POSIX functions
   * @{
   */
  unsigned int melon_sleep(uint32_t secs);
  int melon_usleep(uint64_t usecs);

  ssize_t melon_write(int fildes, const void * data, size_t nbyte, melon_time_t timeout);
  ssize_t melon_pwrite(int fildes, const void * data, size_t nbyte, off_t offset, melon_time_t timeout);
  ssize_t melon_writev(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout);
  ssize_t melon_pwritev(int fildes, const struct iovec *iov, int iovcnt, off_t offset, melon_time_t timeout);

  ssize_t melon_read(int fildes, void * data, size_t nbyte, melon_time_t timeout);
  ssize_t melon_pread(int fildes, void * data, size_t nbyte, off_t offset, melon_time_t timeout);
  ssize_t melon_readv(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout);
  ssize_t melon_preadv(int fildes, const struct iovec *iov, int iovcnt, off_t offset, melon_time_t timeout);

  int melon_connect(int socket, const struct sockaddr *address, socklen_t address_len, melon_time_t timeout);
  int melon_accept(int socket, struct sockaddr * restrict address, socklen_t * restrict address_len, melon_time_t timeout);
  ssize_t melon_sendto(int socket, const void *message, size_t length,
                       int flags, const struct sockaddr *dest_addr,
                       socklen_t dest_len, melon_time_t timeout);
  ssize_t melon_recvfrom(int socket, void *restrict buffer, size_t length,
                         int flags, struct sockaddr * address,
                         socklen_t * address_len, melon_time_t timeout);
  ssize_t melon_recvmsg(int socket, struct msghdr *message, int flags, melon_time_t timeout);
  ssize_t melon_sendmsg(int socket, const struct msghdr *message, int flags, melon_time_t timeout);

  ssize_t melon_sendfile(int out_fd, int in_fd, off_t *offset, size_t count, melon_time_t timeout);

#  ifdef _GNU_SOURCE
  ssize_t melon_splice(int fd_in, loff_t *off_in, int fd_out,
                       loff_t *off_out, size_t len, unsigned int flags, melon_time_t timeout);
  ssize_t melon_tee(int fd_in, int fd_out, size_t len, unsigned int flags, melon_time_t timeout);
  ssize_t melon_vmsplice(int fd, const struct iovec *iov,
                         unsigned long nr_segs, unsigned int flags, melon_time_t timeout);
#  endif

  /** @} */

# ifdef MELON_OVERRIDE_POSIX

#  ifdef sleep
#   undef sleep
#  endif
#  define sleep(Args...) melon_sleep(Args)

#  ifdef usleep
#   undef usleep
#  endif
#  define usleep(Args...) melon_usleep(Args)

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
