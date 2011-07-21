#ifndef MELON_MELONPP_HH
# define MELON_MELONPP_HH

# include <cassert>
# include "melon.h"

namespace melon
{
  class NonCopyable
  {
  private:
    NonCopyable(const NonCopyable &);
  };

  class Melon : private NonCopyable
  {
  public:
    inline Melon(uint16_t nb_threads) { init_succeed = !::melon_init(nb_threads); }
    inline ~Melon() { ::melon_deinit(); }
    inline bool ok() const { return init_succeed_; }
    inline void wait() { ::melon_wait(); }
  private:
    bool init_succeed_;
  };

  template <typename T>
  inline bool startFiber(void (*fct)(T *), T * ctx) { return !::melon_fiber_start((void(*)(void*))fct, (void*)ctx); }
  inline void yield() { ::melon_yield(); }
  inline ::melon_time_t time() { return ::melon_time(); }
  inline ::melon_time_t nanoseconds(int64_t ns) { return ns; }
  inline ::melon_time_t microseconds(int64_t us) { return us * 1000; }
  inline ::melon_time_t milliseconds(int64_t ms) { return ms * 1000 * 1000; }
  inline ::melon_time_t seconds(int64_t s) { return s * 1000 * 1000 * 1000; }
  inline ::melon_time_t minutes(int64_t m) { return m * 60 * 1000 * 1000 * 1000; }
  inline ::melon_time_t hours(int64_t h) { return h * 60 * 60 * 1000 * 1000 * 1000; }
  inline ::melon_time_t days(int64_t d) { return d * 24 * 60 * 60 * 1000 * 1000 * 1000; }

  template <typename T>
  class Locker : private NonCopyable
  {
  public:
    inline ScopedLocker(T & mutex) : mutex_(mutex) { mutex.lock(); }
    inline ~ScopedLocker() { mutex.unlock(); }
  private:
    T & mutex_;
  };

  template <typename T>
  class UniqueLocker : private NonCopyable
  {
  public:
    inline UniqueLocker(T & mutex, bool acquire = true)
      : mutex_(mutex),
        is_locked_(acquire)
    {
      if (acquire)
        mutex_.lock();
    }

    inline ~UniqueLocker() { if (is_locked_) mutex.unlock(); }
    inline void lock() { assert(!is_locked_); mutex_.lock(); }
    inline bool tryLock() { assert(!is_locked_); is_locked_ = mutex_.trylock(); return is_locked_; }
    inline bool timedLock(::melon_time_t time)
    { assert(!is_locked_); is_locked_ = mutex_.timedlock(time); return is_locked_; }
    inline void unlock() { assert(is_locked_); mutex_.unlock(); }
  private:
    bool is_locked_;
    T &  mutex_;
  };

  class Mutex : private NonCopyable
  {
  public:
    typedef Locker<Mutex> Locker;
    typedef UniqueLocker<Mutex> UniqueLocker;
    inline Mutex() : mutex_(::melon_mutex_new()) {}
    inline ~Mutex() { ::melon_mutex_destroy(mutex_); }
    inline bool ok() const { return mutex_; }
    inline void lock() { ::melon_mutex_lock(mutex_); }
    inline void unlock() { ::melon_mutex_lock(mutex_); }
    inline bool tryLock() { return ::melon_mutex_trylock(mutex_); }
    inline bool timedLock(::melon_time_t time) { return ::melon_mutex_timedlock(mutex_, time); }
  private:
    friend class Condition;
    ::melon_mutex * mutex_;
  };

  template <typename T>
  class RwLocker : private NonCopyable
  {
  public:
    enum State { kUnlocked, kReadLocked, kWriteLocked };
    inline RwLocker(T & lock, State acquire = kWriteLocked)
      : state_(acquire), lock_(lock)
    {
      if (acquire == kReadLocked)
        lock_.rdlock();
      else if (acquire == kWriteLocked)
        lock_.wrlock();
    }
    inline ~RwLocker() { if (state_ != kUnlocked) lock_.unlock(); }
  private:
    State  state_;
    RwLock lock_;
  };

  class ReadWriteLock : public NonCopyable
  {
  public:
    typedef ReadWriteLocker<ReadWriteLock> Locker;
    inline ReadWriteLock() : lock_(::melon_rwlock_new()) {}
    inline ~ReadWriteLock() { ::melon_mutex_destroy(lock_); }
    inline bool ok() const { return lock_; }
    inline void readLock() { ::melon_rwlock_rdlock(lock_); }
    inline void tryReadLock() { ::melon_rwlock_tryrdlock(lock_); }
    inline bool timedReadLock(::melon_time_t time) { return ::melon_rwlock_timedrdlock(lock_, time); }
    inline void writelock() { ::melon_rwlock_wrlock(lock_); }
    inline void tryWriteLock() { ::melon_rwlock_trywrlock(lock_); }
    inline bool timedWritelock(::melon_time_t time) { return ::melon_rwlock_timedwrlock(lock_, time); }
    inline void unlock() { ::melon_rwlock_unlock(lock_); }
  private:
    ::melon_rwlock * lock_;
  };

  class Condition : public NonCopyable
  {
  public:
    inline Condition() : cond_(::melon_cond_new()) {}
    inline ~Condition() { ::melon_cond_destroy(cond_); }
    inline void wait(Mutex & mutex) { ::melon_cond_wait(cond_, mutex.mutex_); }
    inline void timedWait(Mutex & mutex, ::melon_time_t time) { ::melon_cond_wait(cond_, mutex.mutex_, time); }
    inline void wakeOne() { ::melon_cond_signal(cond_); }
    inline void wakeAll() { ::melon_cond_broadcast(cond_); }
  private:
    ::melon_cond * cond_;
  };

  void sleep(::melon_time_t time);

  template <typename T, typename QueueType = std::queue<T>>
    class Channel : private NonCopyable
  {
  public:
    inline Channel()
      : closed_(false),
        queue_(),
        mutex_(),
        cond_()
    {
    }

    inline bool ok() const { return mutex_.ok() && cond_.ok(); }
    inline bool push(const T & t)
    {
      Mutex::Locker locker(mutex_);
      if (closed_)
        return false;

      queue_.push_back(t);
      cond_.wakeOne();
      return true;
    }

    inline bool pop(T & t)
    {
      Mutex::Locker locker(mutex_);
      while (true)
      {
        if (!queue_.empty())
        {
          t = queue_.front();
          queue_.pop_front();
          return true;
        }
        else if (closed_)
          return false;
        else
          cond_.wait(mutex_);
      }
    }

    inline void close()
    {
      Mutex::Locker locker(mutex_);
      closed_ = true;
      cond_.wakeAll();
    }

    bool closed_;
    QueueType queue_;
    Mutex mutex_;
    Condition cond_;
  };
}

#endif /* !MELON_MELONPP_HH */
