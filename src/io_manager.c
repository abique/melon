#include <sys/epoll.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "private.h"

int melon_io_manager_init(void)
{
  if (pthread_create(&g_melon.epoll_thread, 0, melon_io_manager_loop, NULL))
    return -1;
  return 0;
}

static void melon_io_manager_handle(struct epoll_event * event)
{
  melon_fiber * curr = g_melon.io[event->data.fd].fibers;
  melon_fiber * next = NULL;

  for (; curr; curr = next)
  {
    if (curr->next == curr ||
        curr->next == g_melon.io[event->data.fd].fibers)
      next = NULL;
    else
      next = curr->next;

    if ((event->events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) ||
        (curr->waited_event & kEventIoRead && (event->events & (EPOLLIN | EPOLLPRI))) ||
        (curr->waited_event & kEventIoWrite && (event->events & (EPOLLOUT))))
    {
      curr->waited_event = kEventNone;
      melon_dlist_unlink(g_melon.io[event->data.fd].fibers, curr, );
      if (curr->timer > 0)
        melon_timer_remove_locked(curr);
      melon_sched_ready_locked(curr);
    }
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
      usleep(50);
      continue;
    }

    int ret = pthread_mutex_lock(&g_melon.lock);
    assert(!ret);
    for (int i = 0; i < count; ++i)
      melon_io_manager_handle(events + i);
    ret = pthread_mutex_unlock(&g_melon.lock);
    assert(!ret);
  }
  return 0;
}

void melon_io_manager_deinit(void)
{
  close(g_melon.epoll_fd);
  g_melon.epoll_fd = -1;
  pthread_cancel(g_melon.epoll_thread);
  pthread_join(g_melon.epoll_thread, NULL);
}

struct waitfor_ctx
{
  melon_fiber * fiber;
  int           fildes;
  int           timeout;
};

static void waitfor_cb(struct waitfor_ctx * ctx)
{
  ctx->timeout = 1;
  melon_dlist_unlink(g_melon.io[ctx->fildes].fibers, ctx->fiber, );
  melon_sched_ready_locked(ctx->fiber);
}

int melon_io_manager_waitfor(int fildes, int waited_events, melon_time_t timeout)
{
  struct waitfor_ctx ctx;
  melon_fiber *      self = g_current_fiber;

  assert(fildes >= 0);
  pthread_mutex_lock(&g_melon.lock);

  struct epoll_event ep_event;
  ep_event.events = (waited_events & kEventIoRead ? EPOLLIN : 0) |
    (waited_events & kEventIoWrite ? EPOLLOUT : 0) |
    EPOLLONESHOT;
  ep_event.data.fd = fildes;
  ctl:
  if (epoll_ctl(g_melon.epoll_fd,
                g_melon.io[fildes].is_in_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_ADD,
                fildes, &ep_event))
  {
    if (errno == EEXIST)
    {
      g_melon.io[fildes].is_in_epoll = 1;
      write(STDERR_FILENO, "critical, should not append\n", 28);
      goto ctl;
    }
    if (errno == ENOENT)
    {
      g_melon.io[fildes].is_in_epoll = 0;
      write(STDERR_FILENO, "critical, don't forget to call melon_close()\n", 45);
      goto ctl;
    }
    return -errno;
  }

  self->waited_event = waited_events;
  self->io_canceled  = 0;
  g_melon.io[fildes].is_in_epoll = 1;
  melon_dlist_push(g_melon.io[fildes].fibers, self, );

  if (timeout > 0)
  {
    ctx.fiber       = self;
    ctx.fildes      = fildes;
    ctx.timeout     = 0;
    self->timer     = timeout;
    self->timer_ctx = &ctx;
    self->timer_cb  = (melon_callback)waitfor_cb;
    melon_timer_push_locked();
  }
  else
  {
    self->timer = 0;
  }
  pthread_mutex_unlock(&g_melon.lock);
  melon_sched_next();

  if (timeout > 0 && ctx.timeout)
  {
    errno = -ETIMEDOUT;
    return -ETIMEDOUT;
  }
  if (self->io_canceled)
  {
    errno = -ECANCELED;
    return -ECANCELED;
  }
  return 0;
}
