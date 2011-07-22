/*!

@mainpage Melon

@section WhatIs What is melon

Melon is a M:N multithreading fiber implementation. It includes transparent I/O reactor,
using the best poller available (epoll, kqueue or iocp). Timeouts are part of the
library for I/O and synchronisation functions.
For exemple read(int fd, void * data, size_t size, melon_time_t timeout).

@section Why Why ?

Have you ever tried google's <a href="http://golang.org">go</a> ?<br>
It's nice because you can create a lot of goroutines write synchronous
code, which behaves like multithreaded asynchronous code with a low overhead.
It makes things really easier. Because writting multithreaded asynchronous code
is error prone.

Melon tries to bring fibers to the C language in a transparent and familiar way,
by using known POSIX API patterns.<br>
So you can refer to standard documentation to know what melon_read() returns etc...
Excepts that our version has 1 more parameter: timeout.
So you don't have to manage timeouts yourself.

@section Cpp If you are looking for a C++ binding

Melon is perfectly usable for a C++ programmer. Buf if you are looking for an
object and RAII approach, then I suggest you to use
<a href="https://github.com/babali/mimosa">Mimosa</a>.

@section ToKnow Things you may know before starting

@li \ref melon_time_t is int64_t type and represent the number of nanoseconds of the MONOTONIC_CLOCK
(one which is not subject to change).
@li every timeout are absolute, you should use melon_time() to get the current time,
and add the amount of desired nanoseconds.
@li synchronisation functions only works in a melon_thread, which means that you can't use them in
the main() function.

@section APIC The C API, POSIX like

The C API has been done to match POSIX as close as possible.
So you can easily try to replace pthreads by melon in your software.

@subsection Initialisation

Before being able to use melon, you have to initialise it.
@include tests/doc-sample.c

@subsection APIMATCH Correspondance between POSIX and melon

Runtime
<table>
<tr>
<td></td>
<td>\ref melon_init</td>
<td>initialise melon</td>
</tr>
<tr>
<td></td>
<td>\ref melon_wait</td>
<td>waits for every fibers to finish</td>
</tr>
<tr>
<td></td>
<td>\ref melon_deinit</td>
<td>deinitialise melon</td>
</tr>
<tr>
<td></td>
<td>\ref melon_kthread_add</td>
<td>adds kernel thread to the thread pool</td>
</tr>
<tr>
<td></td>
<td>\ref melon_kthread_release</td>
<td>removes kernel thread from the thread pool</td>
</tr>
</table>

<br>

Fiber
<table>
<tr>
<td>pthread_t</td>
<td>\ref melon_fiber *</td>
<td></td>
</tr>
<tr>
<td>pthread_create</td>
<td>\ref melon_fiber_start</td>
<td></td>
</tr>
<tr>
<td>pthread_create + pthread_detach</td>
<td>\ref melon_fiber_startlight</td>
<td>if you don't need join the fiber, this one is really faster</td>
</tr>
<tr>
<td>pthread_join</td>
<td>\ref melon_fiber_join</td>
<td></td>
</tr>
<tr>
<td>pthread_detach</td>
<td>\ref melon_fiber_detach</td>
<td></td>
</tr>
<tr>
<td>pthread_tryjoin_np</td>
<td>\ref melon_fiber_tryjoin</td>
<td></td>
</tr>
<tr>
<td>pthread_timedjoin_np</td>
<td>\ref melon_fiber_timedjoin</td>
<td></td>
</tr>
<tr>
<td>pthread_self</td>
<td>\ref melon_fiber_self</td>
<td></td>
</tr>
</table>

<br>

Spinlock
<table>
<tr>
<td>pthread_spinlock_t</td>
<td>\ref melon_spinlock</td>
<td></td>
</tr>
<tr>
<td>pthread_spin_init</td>
<td>\ref melon_spin_init</td>
<td></td>
</tr>
<tr>
<td>pthread_spin_lock</td>
<td>\ref melon_spin_lock</td>
<td></td>
</tr>
<tr>
<td>pthread_spin_unlock</td>
<td>\ref melon_spin_unlock</td>
<td></td>
</tr>
<tr>
<td>pthread_spin_destroy</td>
<td>\ref melon_spin_destroy</td>
<td></td>
</tr>
</table>

<br>

Mutex
<table>
<tr>
<td>pthread_mutex_t *</td>
<td>\ref melon_mutex *</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_init</td>
<td>\ref melon_mutex_new</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_lock</td>
<td>\ref melon_mutex_lock</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_trylock</td>
<td>\ref melon_mutex_trylock</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_timedlock</td>
<td>\ref melon_mutex_timedlock</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_unlock</td>
<td>\ref melon_mutex_unlock</td>
<td></td>
</tr>
<tr>
<td>pthread_mutex_destroy</td>
<td>\ref melon_mutex_destroy</td>
<td></td>
</tr>
</table>

<br>

RWLock
<table>
<tr>
<td>pthread_rwlock_t *</td>
<td>\ref melon_rwlock *</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_init</td>
<td>\ref melon_rwlock_new</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_rdlock</td>
<td>\ref melon_rwlock_rdlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_tryrdlock</td>
<td>\ref melon_rwlock_tryrdlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_timedrdlock</td>
<td>\ref melon_rwlock_timedrdlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_wrlock</td>
<td>\ref melon_rwlock_wrlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_trywrlock</td>
<td>\ref melon_rwlock_trywrlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_timedwrlock</td>
<td>\ref melon_rwlock_timedwrlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_unlock</td>
<td>\ref melon_rwlock_unlock</td>
<td></td>
</tr>
<tr>
<td>pthread_rwlock_destroy</td>
<td>\ref melon_rwlock_destroy</td>
<td></td>
</tr>
</table>

<br>

Condition
<table>
<tr>
<td>pthread_cond_t *</td>
<td>\ref melon_cond *</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_init</td>
<td>\ref melon_cond_new</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_wait</td>
<td>\ref melon_cond_wait</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_timedwait</td>
<td>\ref melon_cond_timedwait</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_signal</td>
<td>\ref melon_cond_signal</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_broadcast</td>
<td>\ref melon_cond_broadcast</td>
<td></td>
</tr>
<tr>
<td>pthread_cond_destroy</td>
<td>\ref melon_cond_destroy</td>
<td></td>
</tr>
</table>

<br>

Barrier
<table>
<tr>
<td>pthread_barrier_t *</td>
<td>\ref melon_barrier *</td>
<td></td>
</tr>
<tr>
<td>pthread_barrier_init</td>
<td>\ref melon_barrier_new</td>
<td></td>
</tr>
<tr>
<td>pthread_barrier_wait</td>
<td>\ref melon_barrier_wait</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_barrier_add</td>
<td>dynamic increase of count</td>
</tr>
<tr>
<td></td>
<td>\ref melon_barrier_release</td>
<td>dynamic decrease of count</td>
</tr>
<tr>
<td>pthread_barrier_destroy</td>
<td>\ref melon_barrier_destroy</td>
<td></td>
</tr>
</table>

<br>

Semaphore
<table>
<tr>
<td></td>
<td>\ref melon_sem *</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_new</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_acquire</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_tryacquire</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_timedacquire</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_release</td>
<td></td>
</tr>
<tr>
<td></td>
<td>\ref melon_sem_destroy</td>
<td></td>
</tr>
</table>

<br>

Sleeps
<table>
<tr>
<td>sleep</td>
<td>\ref melon_sleep</td>
<td></td>
</tr>
<tr>
<td>usleep</td>
<td>\ref melon_usleep</td>
<td></td>
</tr>
</table>

<br>

Input/Output and network related stuff
<table>
<tr>
<td>read</td>
<td>\ref melon_read</td>
<td></td>
</tr>
<tr>
<td>write</td>
<td>\ref melon_write</td>
<td></td>
</tr>
<tr>
<td>pread</td>
<td>\ref melon_pread</td>
<td></td>
</tr>
<tr>
<td>pwrite</td>
<td>\ref melon_pwrite</td>
<td></td>
</tr>
<tr>
<td>readv</td>
<td>\ref melon_readv</td>
<td></td>
</tr>
<tr>
<td>writev</td>
<td>\ref melon_writev</td>
<td></td>
</tr>
<tr>
<td>preadv</td>
<td>\ref melon_preadv</td>
<td></td>
</tr>
<tr>
<td>pwritev</td>
<td>\ref melon_pwritev</td>
<td></td>
</tr>

<tr>
<td>connect</td>
<td>\ref melon_connect</td>
<td></td>
</tr>
<tr>
<td>accept</td>
<td>\ref melon_accept</td>
<td></td>
</tr>
<tr>
<td>sendto</td>
<td>\ref melon_sendto</td>
<td></td>
</tr>
<tr>
<td>recvfrom</td>
<td>\ref melon_recvfrom</td>
<td></td>
</tr>
<tr>
<td>sendmsg</td>
<td>\ref melon_sendmsg</td>
<td></td>
</tr>
<tr>
<td>recvmsg</td>
<td>\ref melon_recvmsg</td>
<td></td>
</tr>
<tr>
<td>sendfile</td>
<td>\ref melon_sendfile</td>
<td></td>
</tr>
</table>

<br>
*/
