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

MELON_MAIN(argc, argv)
{
  (void)argc;
  (void)argv;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
    perror("socket");
    return 1;
  }
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(19044);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd, (struct sockaddr *)&addr, sizeof (addr)))
  {
    melon_close(fd);
    perror("bind");
    return 1;
  }

  if (listen(fd, 5))
  {
    melon_close(fd);
    perror("listen");
    return 1;
  }

  while (1)
  {
    int client = melon_accept(fd, NULL, NULL, 0);
    if (client < 0)
      continue;

    int * data = malloc(sizeof (int));
    if (!data)
    {
      perror("malloc");
      melon_close(client);
      continue;
    }
    *data = client;

    if (melon_fiber_createlight(NULL, handle_client, data))
    {
      free(data);
      melon_close(client);
      continue;
    }
  }
  return 1;
}
