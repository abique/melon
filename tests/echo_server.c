#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <melon/melon.h>

static void * handle_client(void * ctx)
{
  int          fd     = *(int*)ctx;
  const size_t size   = 1024;
  char *       buffer = malloc(size);

  while (1)
  {
    int64_t bytes = melon_read(fd, buffer, size, melon_time() + 5 * MELON_SECOND);
    if (bytes <= 0)
    {
      perror("read");
      break;
    }
    melon_write(fd, buffer, bytes, melon_time() + 5 * MELON_SECOND);
  }
  free(buffer);
  melon_close(fd);
  return NULL;
}

static void * start_server(void * ctx)
{
  (void)ctx;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
    perror("socket");
    return NULL;
  }
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(19044);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd, (struct sockaddr *)&addr, sizeof (addr)))
  {
    close(fd);
    perror("bind");
    return NULL;
  }

  if (listen(fd, 5))
  {
    close(fd);
    perror("listen");
    return NULL;
  }

  while (1)
  {
    int client = accept(fd, NULL, NULL);
    if (client < 0)
      continue;

    int * data = malloc(sizeof (int));
    if (!data)
    {
      perror("malloc");
      close(client);
      continue;
    }
    *data = client;

    if (melon_fiber_startlight(handle_client, data))
    {
      free(data);
      close(client);
      continue;
    }
  }
  return NULL;
}

int main(void)
{
  if (melon_init(0)) // initialises melon, and use the default number of kernel threads
    return 1;

  melon_fiber_startlight(start_server, NULL); // creates at least one fiber
  // you can't use join/detach/lock/... from the main thread, you must be doing it
  // in a fiber thread.

  melon_wait(); // waits for every fibers to terminate
  melon_deinit();
  return 0;
}