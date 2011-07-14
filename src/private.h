#ifndef MELON_PRIVATE_H
# define MELON_PRIVATE_H

# include <pthread.h>
# include <ucontext.h>

# include "melon.h"
# include "spinlock.h"
# include "list.h"
# include "dlist.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef enum MelonEvent
  {
    kEventNone,
    kEventIoRead,
    kEventIoWrite,
    kEventDestroy,
    kEventTimer,
  } MelonEvent;

  /* enum MelonFiberState */
  /* { */
  /*   kInit, */
  /*   kReady, */
  /*   kRunning, */
  /*   kIoWaiting, */
  /*   kMutexWaiting, */
  /*   kCondWaiting, */
  /*   kSleeping, */
  /*   kFinished */
  /* }; */

  typedef void (*melon_callback)(void * ctx);

  struct melon_event
  {
    struct melon_event * next;
    enum MelonEvent      event;
  };

  struct melon_mutex
  {
    melon_spinlock lock;
    melon_fiber *  owner;
    melon_fiber *  lock_queue;
    int            is_recursive;
    int            lock_count;
  };

  struct melon_rwmutex
  {
  };

  struct melon_cond
  {
  };

  typedef struct melon_fiber
  {
    struct melon_fiber * next;  // usefull for intrusive linking
    struct melon_fiber * prev;
    ucontext_t           ctx;
    MelonEvent           waited_event;
    int                  is_detached;
    const char *         name;

    /* start callback */
    melon_callback start_cb;
    void *         start_ctx;

    /* timer stuff */
    melon_time_t         timer;
    struct melon_fiber * timer_next; // usefull for intrusive linking
    struct melon_fiber * timer_prev;
    melon_callback       timer_cb;
    void *               timer_ctx;

    /* after sched_next callback */
    melon_callback       sched_next_cb;
    void *               sched_next_ctx;
  } melon_fiber;

  typedef struct melon
  {
    /* the big melon lock */
    pthread_mutex_t lock;

    /* ready queue */
    melon_fiber *  ready;
    pthread_cond_t ready_cond;

    /* vector of linked list of fiber waiting for io events
     * n producers, 1 consumer */
    melon_fiber ** io_blocked;

    /* epoll stuff */
    int       epoll_fd;
    pthread_t epoll_thread;

    /* thread pool */
    uint32_t       threads_nb;
    pthread_t *    threads;
    melon_fiber ** threads_fiber; // running fiber

    /* the time func */
    pthread_t       timer_thread;
    //pthread_mutex_t timeout_mutex;
    pthread_cond_t  timer_cond;
    melon_fiber *   timer_queue; // sorted

    /* stop the runtime */
    int stop;

    /* the number of fibers */
    int            fibers_count;
    pthread_cond_t fibers_count_zero;
  } melon;

  extern melon g_melon;

  /** Loop which executes ready fibers */
  void * melon_sched_run(void *);
  /** Schedule the next fiber and stops the current (if there is one).
   * The old fiber will not be put in the ready queue like yield would do,
   * so think to a mechanism to wake up the old fiber. */
  void melon_sched_next(void);
  /** lock g_melon.lock and pushes the fiber list in the ready queue */
  void melon_sched_ready(melon_fiber * list);
  /** call this one if you have locked g_melon.lock */
  void melon_sched_ready_locked(melon_fiber * list);

  /** @return 0 on succees */
  int melon_io_manager_init(void);
  void * melon_io_manager_loop(void *);
  void melon_io_manager_deinit(void);

  /** @return 0 on success */
  int melon_timer_manager_init(void);
  void * melon_timer_manager_loop(void *);
  void melon_timer_manager_deinit(void);
  /** pushes the current fiber (\see melon_fiber_self()) in the timer queue
   * using fiber->timeout as absolute time (\see melon_time()). */
  void melon_timer_push();
  /** removes the fiber from the timer_manager's queue.
   * also assume that g_melon.lock is locked */
  void melon_timer_remove_locked(melon_fiber * fiber);

  void melon_fiber_destroy(melon_fiber * fiber);

  int melon_stack_init(void);
  void melon_stack_deinit(void);
  void * melon_stack_alloc(void);
  void melon_stack_free(void * addr);
  uint32_t melon_stack_size(void);

  extern __thread melon_fiber * g_current_fiber;
  extern __thread ucontext_t    g_root_ctx;

# ifdef __cplusplus
}
# endif

#endif /* !MELON_PRIVATE_H */
