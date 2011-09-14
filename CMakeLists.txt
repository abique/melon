cmake_minimum_required(VERSION 2.8)

project(melon C)

enable_testing()

file(STRINGS VERSION MELON_VERSION)
message("melon version: ${MELON_VERSION}")

add_definitions(-D_GNU_SOURCE -D_LARGEFILE64_SOURCE -std=c99 -Wall -Wextra)
include_directories(include)

set(MELON_SRCS
  src/barrier.c
  src/cond.c
  src/fiber.c
  src/io.c
  src/io_manager.c
  src/melon.c
  src/mutex.c
  src/rwlock.c
  src/sched.c
  src/sem.c
  src/stack_alloc.c
  src/stream.c
  src/timer_manager.c
)

add_library(melon SHARED ${MELON_SRCS})
target_link_libraries(melon pthread rt)
set_target_properties(melon PROPERTIES VERSION ${MELON_VERSION} SOVERSION 0)

add_library(smelon STATIC ${MELON_SRCS})
target_link_libraries(smelon pthread rt)
set_target_properties(smelon PROPERTIES OUTPUT_NAME melon)

install(TARGETS melon smelon
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY include/melon DESTINATION include)

add_custom_target(tests ctest)

macro(test_exec NAME SOURCES)
  add_executable(${NAME} EXCLUDE_FROM_ALL ${SOURCES})
  target_link_libraries(${NAME} melon)
  add_dependencies(tests ${NAME})
endmacro(test_exec)

test_exec(list tests/list.c)
add_test(list list)

test_exec(dlist tests/dlist.c)
add_test(dlist dlist)

test_exec(stack_alloc tests/stack_alloc.c)
add_test(stack_alloc stack_alloc)

test_exec(init-n tests/init-n.c)
add_test(init-0 init-n 0)
add_test(init-1 init-n 1)
add_test(init-42 init-n 42)

test_exec(init-reinit tests/init-reinit.c)
add_test(init-reinit init-reinit)

test_exec(create-n-fibers tests/create-n-fibers.c)
add_test(create-0-fibers create-n-fibers 0)
add_test(create-100-fibers create-n-fibers 100)
add_test(create-10000-fibers create-n-fibers 10000)
add_test(create-1000000-fibers create-n-fibers 1000000)

test_exec(melon_sleep tests/melon_sleep.c)
add_test(melon_sleep melon_sleep)

test_exec(mutex_simple tests/mutex_simple.c)
add_test(mutex_simple mutex_simple)

test_exec(cond_signal tests/cond_signal.c)
add_test(cond_signal cond_signal)

test_exec(cond_broadcast tests/cond_broadcast.c)
add_test(cond_broadcast cond_broadcast)

test_exec(cond_timedwait tests/cond_timedwait.c)
add_test(cond_timedwait cond_timedwait)

test_exec(io tests/io.c)
add_test(io io)

test_exec(io-invalid tests/io-invalid.c)
add_test(io-invalid io-invalid)

test_exec(barrier tests/barrier.c)
add_test(barrier barrier)

test_exec(doc-sample samples/hello_world.c)
test_exec(echo_server samples/echo_server.c)
