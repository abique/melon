#include <sys/epoll.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "private.h"

int melon_io_manager_init(void)
{
  g_melon.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (g_melon.epoll_fd < 0)
    return NULL;

  if (pthread_create(&g_melon.io_manager_thread, 0, melon_io_manager_loop, NULL))
  {
    close(g_melon.epoll_fd);
    return -1;
  }
  return 0;
}

void melon_io_manager_deinit(void)
{
  close(g_melon.epoll_fd);
  g_melon.epoll_fd = -1;
  pthread_cancel(g_melon.io_manager_thread);
  pthread_join(g_melon.io_manager_thread, NULL);
}

static void melon_io_manager_release_fiber(melon_fiber * fiber)
{
  if (!fiber)
    return;
  fiber->waited_event = kEventNone;
  if (fiber->timer > 0)
    melon_timer_remove_locked(fiber);
  melon_sched_ready_locked(fiber);
}

static void melon_io_manager_handle(struct epoll_event * event)
{
  melon_fiber * fiber = NULL;

  if (event->events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP))
  {
    // flush everything
#define RELEASE_LOOP(Queue)                     \
    do {                                        \
      melon_dlist_pop(Queue, fiber, );          \
      if (!fiber)                               \
        break;                                  \
      melon_io_manager_release_fiber(fiber);    \
    } while (1)

    RELEASE_LOOP(g_melon.io[event->data.fd].read_queue);
    RELEASE_LOOP(g_melon.io[event->data.fd].write_queue);
#undef RELEASE_LOOP
  }
  if (event->events & (EPOLLIN | EPOLLPRI))
  {
    // release one read from the read queue
    melon_dlist_pop(g_melon.io[event->data.fd].read_queue, fiber, );
    melon_io_manager_release_fiber(fiber);
  }
  if (event->events & (EPOLLOUT))
  {
    // release one read from the write queue
    melon_dlist_pop(g_melon.io[event->data.fd].write_queue, fiber, );
    melon_io_manager_release_fiber(fiber);
  }

  event->events = EPOLLONESHOT |
    (g_melon.io[event->data.fd].read_queue ? EPOLLIN : 0) |
    (g_melon.io[event->data.fd].write_queue ? EPOLLOUT : 0);
  if (event->events != EPOLLONESHOT)
    epoll_ctl(g_melon.epoll_fd, EPOLL_CTL_MOD, event->data.fd, event);
}

void * melon_io_manager_loop(void * dummy)
{
  (void)dummy;

  struct epoll_event events[256];

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

struct waitfor_ctx
{
  melon_fiber * fiber;
  int           fildes;
  int           timeout;
};

static void waitfor_cb(struct waitfor_ctx * ctx)
{
  ctx->timeout = 1;
  if (ctx->fiber->waited_event & kEventIoRead)
    melon_dlist_unlink(g_melon.io[ctx->fildes].read_queue, ctx->fiber, );
  else if (ctx->fiber->waited_event & kEventIoWrite)
    melon_dlist_unlink(g_melon.io[ctx->fildes].write_queue, ctx->fiber, );
  melon_sched_ready_locked(ctx->fiber);
}

int melon_io_manager_waitfor(int fildes, int waited_event, melon_time_t timeout)
{
  struct waitfor_ctx ctx;
  melon_fiber *      self = g_current_fiber;

  if (fildes < 0)
  {
    errno = EINVAL;
    return -1;
  }

  pthread_mutex_lock(&g_melon.lock);

  struct epoll_event ep_event;
  ep_event.events = EPOLLONESHOT | EPOLLET |
    (g_melon.io[fildes].read_queue ? EPOLLIN : 0) |
    (g_melon.io[fildes].write_queue ? EPOLLOUT : 0) |
    (waited_event & kEventIoRead ? EPOLLIN : 0) |
    (waited_event & kEventIoWrite ? EPOLLOUT : 0);
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
    pthread_mutex_unlock(&g_melon.lock);
    return -1;
  }

  self->waited_event = waited_event;
  self->io_canceled  = 0;
  g_melon.io[fildes].is_in_epoll = 1;
  if (waited_event & kEventIoRead)
    melon_dlist_push(g_melon.io[fildes].read_queue, self, );
  else if (waited_event & kEventIoWrite)
    melon_dlist_push(g_melon.io[fildes].write_queue, self, );
  else
    assert(0 && "invalid waited_event");

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
