#ifndef MELON_PRIVATE_H
# define MELON_PRIVATE_H

# include <pthread.h>
# include <ucontext.h>

# include "melon.h"

# define UNUSED __attribute__((unused))

# ifdef __cplusplus
extern "C" {
# endif

  enum MelonEvent
  {
    kNone,
    kIoRead,
    kIoWrite,
  };

  enum MelonFiberState
  {
    kInit,
    kReady,
    kRunning,
    kIoWaiting,
    kMutexWaiting,
    kCondWaiting,
    kSleeping,
    kFinished
  };

  struct melon_event
  {
    struct melon_event * next;
    enum MelonEvent      event;
  };

  struct melon_fiber
  {
    struct melon_fiber * next;
    enum MelonFiberState state;
    ucontext_t           ctx;
    enum MelonEvent      waited_event;
  };

  struct melon
  {
    pthread_mutex_t mutex;

    int epoll_fd;
    pthread_t epoll_thread;

    /* ready queue */
    struct melon_fiber * ready_head;
    struct melon_fiber * ready_tail;

    /* vector of linked list of fiber waiting for io events
     * n producers, 1 consumer */
    struct melon_fiber **  io_blocked;

    uint32_t    threads_nb;
    pthread_t * threads;
  };

  struct melon_mutex
  {
  };

  struct melon_rwmutex
  {
  };

  struct melon_cond
  {
  };

  extern struct melon g_melon;

  void * melon_sched_run(void *);
  int melon_sched_next();

  void melon_io_manager_init();
  void melon_io_manager_loop(void *)
  void melon_io_manager_deinit();

# ifdef __cplusplus
}
# endif

#endif /* !MELON_PRIVATE_H */
