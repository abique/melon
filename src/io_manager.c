#include <sys/epoll.h>
#include <assert.h>

#include "private.h"

int melon_io_manager_init(void)
{
  if (pthread_create(&g_melon.epoll_thread, 0, melon_io_manager_loop, NULL))
    return -1;
  return 0;
}

static void melon_io_manager_handle(struct epoll_event * event)
{
  melon_fiber * fiber     = g_melon.io_blocked[event->data.fd];
  melon_fiber * tmp_fiber = NULL;

  while (fiber)
  {
    if ((event->events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) ||
        (fiber->waited_event == kIoRead && (event->events & (EPOLLIN | EPOLLPRI))) ||
        (fiber->waited_event == kIoWrite && (event->events & (EPOLLOUT))))
    {
      tmp_fiber           = fiber;
      fiber->waited_event = kNone;
      fiber               = fiber->next;
      melon_list_push(g_melon.ready, tmp_fiber);
    }
    else
      fiber = fiber->next;
  }
}

void * melon_io_manager_loop(void * dummy)
{
  (void)dummy;

  struct epoll_event events[256];

  g_melon.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (g_melon.epoll_fd < 0)
    return NULL;

  while (!g_melon.stop)
  {
    int count = epoll_wait(g_melon.epoll_fd, events,
                           sizeof (events) / sizeof (events[0]), -1);

    if (count < 0)
    {
      if (g_melon.stop)
        return NULL;
      melon_yield();
      continue;
    }

    int ret = pthread_mutex_lock(&g_melon.io_blocked_mutex);
    assert(!ret);
    for (i = 0; i < count; ++i)
      melon_io_manager_handle(events + i);
    ret = pthread_mutex_unlock(&g_melon.io_blocked_mutex);
    assert(!ret);
  }
  return 0;
}

void melon_io_manager_deinit(void)
{
  close(g_melon.epoll_fd);
  g_melon.epoll_fd = -1;
  pthread_join(g_melon.epoll_thread, NULL);
}
