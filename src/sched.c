#include "private.h"

void * melon_sched_run(void * dummy __attribute__((unused)))
{
  while (melon_sched_next())
    continue;
  return 0;
}

int melon_sched_next()
{
  return 1;
}
