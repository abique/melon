cmake_minimum_required(VERSION 2.8)

project(melon C)

enable_testing()

add_definitions(-D_GNU_SOURCE -std=c99 -Wall -Wextra)

add_library(melon
  src/melon.c
  src/sched.c
  src/fiber.c
  src/io_manager.c
  src/timer_manager.c
  src/stack_alloc.c
  src/mutex.c
)
target_link_libraries(melon pthread rt)

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
add_test(mutex_simple tests/mutex_simple.c)
