#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "private.h"

int melon_io_manager_init(void)
{
  g_melon.kqueue_fd = kqueue();
  if (g_melon.kqueue_fd < 0)
    return -1;

  if (pthread_create(&g_melon.io_manager_thread, 0, melon_io_manager_loop, NULL))
    return -1;
  return 0;
}

void melon_io_manager_deinit(void)
{
  close(g_melon.kqueue_fd);
  g_melon.kqueue_fd = -1;
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

static void melon_io_manager_handle(struct kevent * event,
				    struct kevent ** new_event)
{
  melon_fiber * fiber = NULL;

  if (event->flags & (EV_ERROR | EV_EOF))
  {
    // flush everything
#define RELEASE_LOOP(Queue)                     \
    do {                                        \
      melon_dlist_pop(Queue, fiber, );          \
      if (!fiber)                               \
        break;                                  \
      melon_io_manager_release_fiber(fiber);    \
    } while (1)

    RELEASE_LOOP(g_melon.io[event->ident].read_queue);
    RELEASE_LOOP(g_melon.io[event->ident].write_queue);
#undef RELEASE_LOOP
  }
  if (event->filter & (EVFILT_READ))
  {
    // release one read from the read queue
    melon_dlist_pop(g_melon.io[event->ident].read_queue, fiber, );
    melon_io_manager_release_fiber(fiber);
  }
  if (event->filter & (EVFILT_WRITE))
  {
    // release one read from the write queue
    melon_dlist_pop(g_melon.io[event->ident].write_queue, fiber, );
    melon_io_manager_release_fiber(fiber);
  }

  if (g_melon.io[event->ident].read_queue ||
      g_melon.io[event->ident].write_queue)
  {
    (*new_event)->ident = event->ident;
    (*new_event)->filter =
      (g_melon.io[event->ident].read_queue ? EVFILT_READ : 0) |
      (g_melon.io[event->ident].write_queue ? EVFILT_WRITE : 0);
    (*new_event)->filter = 0;
    (*new_event)->flags = EV_ONESHOT | EV_ADD | EV_EOF | EV_ERROR;
    (*new_event)->fflags = 0;
    (*new_event)->data = 0;
    (*new_event)->udata = NULL;
    ++*new_event;
  }
}

void * melon_io_manager_loop(void * dummy)
{
  (void)dummy;

  struct kevent events[256];
  struct kevent * new_events = events;

  while (!g_melon.stop)
  {
    int count = kevent(g_melon.kqueue_fd, new_events, new_events - events, events,
		       sizeof (events) / sizeof (events[0]), NULL);

    if (count < 0)
    {
      if (g_melon.stop)
        return NULL;
      usleep(50);
      continue;
    }

    new_events = events;
    int ret = pthread_mutex_lock(&g_melon.lock);
    assert(!ret);
    for (int i = 0; i < count; ++i)
      melon_io_manager_handle(events + i, &new_events);
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

  struct kevent event;
  event.ident = fildes;
  event.filter =
    (g_melon.io[fildes].read_queue ? EVFILT_READ : 0) |
    (g_melon.io[fildes].write_queue ? EVFILT_WRITE : 0) |
    (waited_event & kEventIoRead ? EVFILT_READ : 0) |
    (waited_event & kEventIoWrite ? EVFILT_WRITE : 0);
  event.flags = EV_ONESHOT | EV_ADD | EV_EOF | EV_ERROR;
  event.fflags = 0;
  event.data = 0;
  event.udata = NULL;
  if (kevent(g_melon.kqueue_fd, &event, 1, NULL, 0, NULL) == -1)
  {
    write(STDERR_FILENO, "critical, kevent() error\n", 25);
    pthread_mutex_unlock(&g_melon.lock);
    return -1;
  }

  self->waited_event = waited_event;
  self->io_canceled  = 0;
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
