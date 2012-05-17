#ifndef MELON_PRIVATE_H
# define MELON_PRIVATE_H

# include <pthread.h>

# if __unix__
#  include <ucontext.h>
# elif __WIN32__
#  include "ucontext_win32.h"
# else
#  error "no makecontext() on this plateform"
# endif

# include "melon.h"
# include "spinlock.h"
# include "list.h"
# include "dlist.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef enum MelonEvent
  {
    kEventNone    = 0x0,
    kEventIoRead  = 0x1,
    kEventIoWrite = 0x2,
    kEventDestroy = 0x4,
    kEventTimer   = 0x8,
  } MelonEvent;

  typedef void (*melon_callback)(void * ctx);

  struct melon_event
  {
    struct melon_event * next;
    enum MelonEvent      event;
  };

# define MUTEX_ATTR                             \
  int is_recursive

  struct melon_mutexattr
  {
    MUTEX_ATTR;
  };

  struct melon_mutex
  {
    melon_spinlock lock;
    melon_fiber *  owner;
    melon_fiber *  lock_queue; /* dlist for fast removal in timedlock */
    int            lock_count;

    MUTEX_ATTR;
  };

  struct melon_rwlock
  {
    melon_mutex * lock;
    melon_cond *  wcond;
    melon_cond *  rcond;
    int           lock_count;
    int           is_read;
    melon_fiber * wowner;
  };

  struct melon_cond
  {
    melon_spinlock lock;
    melon_fiber *  wait_queue;
  };

# define SEM_ATTR                               \
  uint32_t left

  struct melon_semattr
  {
    SEM_ATTR;
  };

  struct melon_sem
  {
    melon_mutex * lock;
    melon_cond  * cond;
    melon_fiber * queue;
    uint32_t      nb; // sanity check

    SEM_ATTR;
  };

# define BARRIER_ATTR                           \
  uint32_t count

  struct melon_barrierattr
  {
    BARRIER_ATTR;
  };

  struct melon_barrier
  {
    melon_mutex * lock;
    melon_cond *  cond;

    BARRIER_ATTR;
  };

# define FIBER_ATTR                             \
  const char *   name;                          \
  uint32_t       stack_size

  struct melon_fiberattr
  {
    FIBER_ATTR;
  };

  struct melon_fiber
  {
    melon_spinlock lock; // locked while running the fiber so it can't be ran two times
    melon_fiber *  next; // usefull for intrusive linking
    melon_fiber *  prev;
    ucontext_t     ctx;
    MelonEvent     waited_event;
    int            io_canceled;

    /* start callback */
    void *         (*start_cb)(void *);
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

    /* while waiting on a condition, we save the locked mutex */
    melon_mutex *        cond_mutex;

    /* restore the lock count on cond_signal() */
    int                  lock_count;

    /* the semaphore nb res to acquire */
    int                  sem_nb;

    /* join stuff */
    void *               join_retval;
    melon_mutex *        join_mutex;
    melon_cond *         join_cond;
    int                  join_is_detached;
    int                  join_is_finished;

    FIBER_ATTR;
  };

  typedef struct melon_fd
  {
    melon_fiber * read_queue;  // linked list of fibers waiting for a read event
    melon_fiber * write_queue; // linked list of fibers waiting for a write event
#if __linux__
    int           is_in_epoll; // is in epoll
# endif
  } melon_fd;

  typedef struct melon
  {
    /* the big melon lock */
    pthread_mutex_t lock;

    /* ready queue */
    melon_fiber *  ready;
    pthread_cond_t ready_cond;

    /* vector of io ctx */
    melon_fd * io;

    /* io_manager stuff */
# ifdef __linux__
    int       epoll_fd;
# endif
# ifdef __FreeBSD__
    int       kqueue_fd;
# endif
    pthread_t io_manager_thread;

    /* thread pool */
    uint32_t       threads_nb;
    pthread_t *    threads;
    melon_fiber ** threads_fiber; // running fiber

    /* the time func */
    pthread_t       timer_thread;
    //pthread_mutex_t timeout_mutex;
    pthread_cond_t  timer_cond;
    melon_fiber *   timer_queue; // dlist sorted

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
  int melon_io_manager_waitfor(int fildes, int waited_event, melon_time_t timeout);

  /** @return 0 on success */
  int melon_timer_manager_init(void);
  void * melon_timer_manager_loop(void *);
  void melon_timer_manager_deinit(void);
  /** pushes the current fiber (\see melon_fiber_self()) in the timer queue
   * using fiber->timeout as absolute time (\see melon_time()). */
  void melon_timer_push(void);
  /** same as above, but you have to call melon_sched_next() */
  void melon_timer_push_locked(void);
  /** removes the fiber from the timer_manager's queue.
   * also assume that g_melon.lock is locked */
  void melon_timer_remove_locked(melon_fiber * fiber);

  void melon_fiber_destroy(melon_fiber * fiber);

  int melon_stack_init(void);
  void melon_stack_deinit(void);
  void * melon_stack_alloc(void);
  void melon_stack_free(void * addr);
  uint32_t melon_stack_size(void);

  void melon_mutex_lock2(melon_fiber * fiber, melon_mutex * mutex);

  extern __thread melon_fiber * g_current_fiber;
  extern __thread ucontext_t    g_root_ctx;

# ifdef __cplusplus
}
# endif

#endif /* !MELON_PRIVATE_H */
