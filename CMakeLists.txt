cmake_minimum_required(VERSION 2.8)

project(melon C)

add_definitions(-D_GNU_SOURCE -std=c99 -Wall -Wextra)

add_library(melon
  src/melon.c
  src/sched.c
  src/fiber.c

  #src/io_manager.c
)
