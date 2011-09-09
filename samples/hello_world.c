#include <unistd.h>
#include <errno.h>
#include <melon/melon.h>

void * my_fiber(void * ctx)
{
  (void)ctx;
  char buffer[128];

  int nbytes = melon_read(STDIN_FILENO, buffer, sizeof (buffer), melon_time() + 2 * MELON_SECOND);
  if (nbytes < 0 && errno == -ETIMEDOUT)
    melon_write(STDERR_FILENO, "timedout\n", 9, 0);
  else if (nbytes < 0)
    melon_write(STDERR_FILENO, "error\n", 6, 0);
  else
    melon_write(STDOUT_FILENO, buffer, nbytes, 0);
  return NULL;
}

int main(void)
{
  if (melon_init(0)) // initialises melon, and use the default number of kernel threads
    return 1;

  melon_fiber_createlight(NULL, my_fiber, NULL); // creates at least one fiber
  // you can't use join/detach/lock/... from the main thread, you must be doing it
  // in a fiber thread.

  melon_wait(); // waits for every fibers to terminate
  melon_deinit();
  return 0;
}
