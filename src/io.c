#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "private.h"

#define IMPLEMENT_IO(Event, Type, Name, Args...)                \
  Type melon_##Name(Args)                                       \
  {                                                             \
    int ret = melon_io_manager_waitfor(fildes, Event, timeout); \
    if (ret)                                                    \
      return -1;                                                \
    return Name(Args);                                          \
  }

ssize_t melon_write(int fildes, const void * data, size_t nbyte, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return write(fildes, data, nbyte);
}

ssize_t melon_pwrite(int fildes, const void * data, size_t nbyte, off_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return pwrite(fildes, data, nbyte, offset);
}

ssize_t melon_writev(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return writev(fildes, iov, iovcnt);
}

ssize_t melon_pwritev(int fildes, const struct iovec *iov, int iovcnt, off_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoWrite, timeout))
    return -1;
  return pwrite(fildes, iov, iovcnt, offset);
}

ssize_t melon_read(int fildes, void * data, size_t nbyte, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return read(fildes, data, nbyte);
}

ssize_t melon_pread(int fildes, void * data, size_t nbyte, off_t offset, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return pread(fildes, data, nbyte, offset);
}

ssize_t melon_readv(int fildes, const struct iovec *iov, int iovcnt, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(fildes, kEventIoRead, timeout))
    return -1;
  return readv(fildes, iov, iovcnt);
}

ssize_t melon_preadv(int fildes, const struct iovec *iov, int iovcnt, off_t offset, melon_time_t timeout)
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

ssize_t melon_sendto(int socket, const void *message, size_t length,
                     int flags, const struct sockaddr *dest_addr,
                     socklen_t dest_len, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoWrite, timeout))
    return -1;
  return sendto(socket, message, length, flags, dest_addr, dest_len);
}

ssize_t melon_recvfrom(int socket, void *restrict buffer, size_t length,
                       int flags, struct sockaddr * address,
                       socklen_t * address_len, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoRead, timeout))
    return -1;
  return recvfrom(socket, buffer, length, flags, address, address_len);
}

ssize_t melon_recvmsg(int socket, struct msghdr *message, int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoRead, timeout))
    return -1;
  return recvmsg(socket, message, flags);
}

ssize_t melon_sendmsg(int socket, const struct msghdr *message, int flags, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(socket, kEventIoWrite, timeout))
    return -1;
  return sendmsg(socket, message, flags);
}

ssize_t melon_sendfile(int out_fd, int in_fd, off_t *offset, size_t count, melon_time_t timeout)
{
  if (melon_io_manager_waitfor(in_fd, kEventIoRead, timeout))
    return -1;
  if (melon_io_manager_waitfor(out_fd, kEventIoWrite, timeout))
    return -1;
  return sendfile(out_fd, in_fd, offset, count);
}
