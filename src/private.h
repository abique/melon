#ifndef MELON_PRIVATE_H
# define MELON_PRIVATE_H

# include <pthread.h>
# include <ucontext.h>

# include "melon.h"

# ifdef __cplusplus
extern "C" {
# endif

  typedef enum MelonEvent
  {
    kEventNone,
    kEventIoRead,
    kEventIoWrite,
    kEventJoin, // the fiber terminate and is waiting for another fiber to join it
    kEventTerminate, // the fiber joined another and is waiting for the other to terminate
    kEventDestroy,
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

  typedef struct melon_event
  {
    struct melon_event * next;
    enum MelonEvent      event;
  } melon_event;

  typedef struct melon_mutex
  {
  } melon_mutex;

  typedef struct melon_rwmutex
  {
  } melon_rwmutex;

  typedef struct melon_cond
  {
  } melon_cond;

  typedef struct melon_fiber
  {
    struct melon_fiber * next; // usefull for intrusive linking
    uint64_t             timeout;
    ucontext_t           ctx;
    MelonEvent           waited_event;
    int                  is_detached;
    struct melon_fiber * terminate_waiter;
    const char *         name;
    void                 (*callback)(void * ctx);
    void *               callback_ctx;
    pthread_spinlock_t   lock; // used for join
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
    melon_time_f time_func;
    void *       time_ctx;
    pthread_t    timer_thread;
    melon_time_t timer_resolution;

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
   * so think to a mechanism to wake up the old fiber.
   * @param destroy_current_fiber will destroy the current fiber */
  void melon_sched_next(int destroy_current_fiber);
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

  void melon_fiber_destroy(melon_fiber * fiber);

  int melon_stack_init();
  void melon_stack_deinit();
  void * melon_stack_alloc();
  void melon_stack_free(void * addr);
  uint32_t melon_stack_size();

  extern __thread melon_fiber * g_current_fiber;
  extern __thread ucontext_t    g_root_ctx;

  // circular linked list
# define melon_list_push(List, Item)            \
  do {                                          \
    if (!(List))                                \
    {                                           \
      (List) = (Item);                          \
      (Item)->next = (Item);                    \
    }                                           \
    else                                        \
    {                                           \
      (Item)->next = (List)->next;              \
      (List)->next = (Item);                    \
      (List) = (Item);                          \
    }                                           \
  } while (0)

# define melon_list_pop(List, Item)             \
  do {                                          \
    /* empty list */                            \
    if (!(List))                                \
      (Item) = NULL;                            \
    /* just 1 element */                        \
    else if ((List) == (List)->next ||          \
             (List)->next == NULL)              \
    {                                           \
      (Item) = (List);                          \
      (Item)->next = NULL;                      \
      (List) = NULL;                            \
    }                                           \
    /* many elements */                         \
    else                                        \
    {                                           \
      (Item) = (List)->next;                    \
      (List)->next = (Item)->next;              \
      (Item)->next = NULL;                      \
    }                                           \
  } while (0)

# ifdef __cplusplus
}
# endif

#endif /* !MELON_PRIVATE_H */
