PROJECT(melon C)

ADD_DEFINITIONS(-D_GNU_SOURCE)

ADD_LIBRARY(melon
  src/io_manager.c
  src/melon.c
  src/sched.c)
