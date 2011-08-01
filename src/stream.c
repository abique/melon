#include <assert.h>
#include <stdlib.h>

#include "melon.h"

typedef struct
{
  int                  fd;
  const melon_time_t * timeout;
} stream_cookie;

static ssize_t cookie_read(stream_cookie * cookie, char *buf, size_t size)
{
  return melon_read(cookie->fd, buf, size, cookie->timeout ? *cookie->timeout : 0);
}

static ssize_t cookie_write(stream_cookie * cookie, const char *buf, size_t size)
{
  return melon_write(cookie->fd, buf, size, cookie->timeout ? *cookie->timeout : 0);
}

static int cookie_seek(stream_cookie * cookie, off64_t * offset, int whence)
{
  off64_t ret = lseek64(cookie->fd, *offset, whence);
  if (ret == -1)
    return -1;
  *offset = ret;
  return 0;
}

static int cookie_close(stream_cookie * cookie)
{
  melon_close(cookie->fd);
  free(cookie);
  return 0;
}

static const cookie_io_functions_t io_funcs = {
  .read  = (cookie_read_function_t*)&cookie_read,
  .write = (cookie_write_function_t*)&cookie_write,
  .seek  = (cookie_seek_function_t*)&cookie_seek,
  .close = (cookie_close_function_t*)&cookie_close
};

static int scan_mode(const char * mode)
{
  assert(mode);
  switch (mode[0])
  {
  case 'a':
    if (mode[1] == '+')
      return O_APPEND | O_CREAT;
    return O_APPEND | O_CREAT | O_RDWR;
  case 'r':
    if (mode[1] == '+')
      return O_RDONLY;
    return O_RDWR;
  case 'w':
    if (mode[1] == '+')
      return O_WRONLY | O_CREAT | O_TRUNC;
    return O_RDWR | O_CREAT | O_TRUNC;
  default:
    assert(0);
    return 0;
  }
}

FILE * melon_fopen(const char * path, const char * mode, const melon_time_t * timeout)
{
  stream_cookie * cookie = malloc(sizeof (*cookie));
  cookie->fd             = open(path, scan_mode(mode), 0644);
  cookie->timeout        = timeout;
  FILE *          stream = fopencookie(cookie, mode, io_funcs);
  if (stream)
    return stream;
  close(cookie->fd);
  free(cookie);
  return NULL;
}

FILE * melon_fdopen(int fd, const char * mode, const melon_time_t * timeout)
{
  stream_cookie * cookie = malloc(sizeof (*cookie));
  cookie->fd             = fd;
  cookie->timeout        = timeout;
  FILE *          stream = fopencookie(cookie, mode, io_funcs);
  if (stream)
    return stream;
  free(cookie);
  return NULL;
}
