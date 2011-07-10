#include <stdlib.h>

#include "../src/melon.h"

int main(int argc, char ** argv)
{
  if (argc != 2)
    return 1;

  int n = atoi(argv[1]);
  if (melon_init(n))
    return 1;

  melon_deinit();
  return 0;
}
