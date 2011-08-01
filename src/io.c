#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
# define _LARGEFILE64_SOURCE
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "private.h"

int melon_close(int fildes)
{
  assert(fildes >= 0);
  int ret = pthread_mutex_lock(&g_melon.lock);
  assert(!ret);

  melon_fiber * curr = g_melon.io[fildes].fibers;
  while (1)
  {
    melon_dlist_pop(g_melon.io[fildes].fibers, curr, );
    if (!curr)
      break;
    curr->waited_event = kEventNone;
    if (curr->timer > 0)
      melon_timer_remove_locked(curr);
    curr->io_canceled = 1;
    melon_sched_ready_locked(curr);
  }
  g_melon.io[fildes].is_in_epoll = 0;

  pthread_mutex_unlock(&g_melon.lock);
  return close(fildes);
}

int64_t melon_write(int fildes, const void * data, uint64_t nbyte, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return write(fildes, data, nbyte);
}

int64_t melon_pwrite(int fildes, const void * data, uint64_t nbyte, int64_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return pwrite(fildes, data, nbyte, offset);
}

int64_t melon_writev(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return writev(fildes, iov, iovcnt);
}

int64_t melon_pwritev(int fildes, const struct iovec *iov, int iovcnt, int64_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return pwrite(fildes, iov, iovcnt, offset);
}

int64_t melon_read(int fildes, void * data, uint64_t nbyte, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return read(fildes, data, nbyte);
}

int64_t melon_pread(int fildes, void * data, uint64_t nbyte, int64_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return pread(fildes, data, nbyte, offset);
}

int64_t melon_readv(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return readv(fildes, iov, iovcnt);
}

int64_t melon_preadv(int fildes, const struct iovec *iov, int iovcnt, int64_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return preadv(fildes, iov, iovcnt, offset);
}

int melon_connect(int socket, const struct sockaddr *address, socklen_t address_len, melon_time_t timeout)
{
  fcntl(socket, F_SETFL, O_NONBLOCK);
  if (!connect(socket, address, address_len))
    return 0;

  if (errno != EINPROGRESS)
    return -1;

  if (melon_io_manager_waitfor(socket, kEventIoWrite, timeout))
    return -1;
  return 0;
}

int melon_accept(int socket, struct sockaddr * restrict address, socklen_t * restrict address_len, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoRead, timeout))
    return -1;
  return accept(socket, address, address_len);
}

int64_t melon_sendto(int socket, const void *message, uint64_t length,
                     int flags, const struct sockaddr *dest_addr,
                     socklen_t dest_len, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoWrite, timeout))
    return -1;
  return sendto(socket, message, length, flags, dest_addr, dest_len);
}

int64_t melon_recvfrom(int socket, void *restrict buffer, uint64_t length,
                       int flags, struct sockaddr * address,
                       socklen_t * address_len, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoRead, timeout))
    return -1;
  return recvfrom(socket, buffer, length, flags, address, address_len);
}

int64_t melon_recvmsg(int socket, struct msghdr *message, int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoRead, timeout))
    return -1;
  return recvmsg(socket, message, flags);
}

int64_t melon_sendmsg(int socket, const struct msghdr *message, int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoWrite, timeout))
    return -1;
  return sendmsg(socket, message, flags);
}

int64_t melon_sendfile(int out_fd, int in_fd, int64_t *offset, uint64_t count, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(in_fd, kEventIoRead, timeout))
    return -1;
  if (melon_io_manager_waitfor(out_fd, kEventIoWrite, timeout))
    return -1;
  return sendfile(out_fd, in_fd, offset, count);
}

int64_t melon_splice(int fd_in, int64_t *off_in, int fd_out,
                     int64_t *off_out, uint64_t len, unsigned int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fd_in, kEventIoRead, timeout))
    return -1;
  if (melon_io_manager_waitfor(fd_out, kEventIoWrite, timeout))
    return -1;
  return splice(fd_in, off_in, fd_out, off_out, len, flags);
}

int64_t melon_tee(int fd_in, int fd_out, uint64_t len, unsigned int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fd_in, kEventIoRead, timeout))
    return -1;
  if (melon_io_manager_waitfor(fd_out, kEventIoWrite, timeout))
    return -1;
  return tee(fd_in, fd_out, len, flags);
}

int64_t melon_vmsplice(int fd, const struct iovec *iov,
                       unsigned long nr_segs, unsigned int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fd, kEventIoRead, timeout))
    return -1;
  return vmsplice(fd, iov, nr_segs, flags);
}
