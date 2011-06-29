#include "private.h"

void * melon_sched_run(void *)
{
  while (melon_sched_next())
    continue;
  return 0;
}

int melon_sched_next()
{
  return true;
}
