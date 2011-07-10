cmake_minimum_required(VERSION 2.8)

project(melon C)

add_definitions(-D_GNU_SOURCE -std=c99 -Wall -Wextra)

add_library(melon
  src/melon.c
  src/sched.c
  src/fiber.c
  src/io_manager.c
)
target_link_libraries(melon pthread)

add_custom_target(tests)

macro(test NAME SOURCES)
  add_executable(${NAME} EXCLUDE_FROM_ALL ${SOURCES})
  target_link_libraries(${NAME} melon)
  add_test(test-${NAME} ${NAME})
  add_dependencies(tests test-${NAME})
endmacro(test)

test(init-0 tests/init-0.c)
test(init-42 tests/init-42.c)
