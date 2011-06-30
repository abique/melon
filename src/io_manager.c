#include <sys/epoll.h>

#include "private.h"

void melon_io_manager_init()
{
  int ret = pthread_create(&g_melon.epoll_thread, 0, melon_io_manager_loop, 0);
  assert(!ret);
}

static void melon_io_manager_handle(struct epoll_event * event)
{
  melon_fiber ** fiber = g_melon.io_blocked + event->fd;

  while (*fiber)
  {
    if ((event->events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) ||
        ((*fiber)->waited_event == kIoRead && (event->events & (EPOLLIN | EPOLLPRI))) ||
        ((*fiber)->waited_event == kIoWrite && (event->events & (EPOLLOUT))))
    {
      (*fiber)->waited_event = kNone;

      if (!g_melon.ready_head)
      {
        g_melon.ready_head = *fiber;
        g_melon.ready_tail = *fiber;
      }
      else
      {
        assert(!g_melon.ready_tail);
        g_melon.ready_tail->next = *fiber;
        g_melon.ready_tail = *fiber;
      }

      *fiber = (*fiber)->next;
      g_melon.ready_tail->next = 0;
    }
    else
      fiber = &(*fiber)->next;
  }
}

void * melon_io_manager_loop(void * dummy UNUSED)
{
  struct epoll_event events[256];

  g_melon.io_manager.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (g_melon.io_manager.epoll_fd < 0)
  {
    // TODO: handle error
    assert(g_melon.io_manager.epoll_fd >= 0);
    return 0;
  }

  while (!g_melon.io_manager.stop)
  {
    int count = epoll_wait(g_melon.io_manager.epoll_fd, events,
                           sizeof (events) / sizeof (events[0]), -1);

    if (count < 0)
    {
      if (g_melon.io_manager.stop)
        return 0;
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

void melon_io_manager_deinit()
{
}
