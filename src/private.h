#ifndef MELON_PRIVATE_H
# define MELON_PRIVATE_H

# include <pthread.h>
# include <sys/epoll.h>
# include <ucontext.h>

# include "melon.h"

# ifdef __cplusplus
extern "C" {
# endif

  struct melon_io_manager
  {
    int                epoll_fd;
    struct epoll_event events[256];
    pthread_t          thread;
  };

  enum MelonEvent
  {
    IoRead,
    IoWrite,
    IoClose,
    IoError,
    MutexUnlock,
    CondSignal,
  };

  struct melon_event
  {
    struct melon_event * next;
    enum MelonEvent      event;
  };

  struct melon_fiber
  {
    ucontext_t ctx;
  };

  struct melon
  {
    struct melon_io_manager io_manager;

    pthread_mutex_t      events_mutex;
    pthread_cond_t       events_cond;
    struct melon_event * events_head;
    struct melon_event * events_tail;

    uint32_t    threads_nb;
    pthread_t * threads;
  };

  struct melon_mutex
  {
  };

  struct melon_cond
  {
  };

  extern struct melon g_melon;

  void melon_io_manager_init(struct melon_io_manager * io_manager);
  void melon_io_manager_deinit(struct melon_io_manager * io_manager);

  void * melon_sched_run(void *);
  int melon_sched_next();

# ifdef __cplusplus
}
# endif

#endif /* !MELON_PRIVATE_H */
