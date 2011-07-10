#include "../src/melon.h"

int main()
{
  if (melon_init(0))
    return 1;

  melon_deinit();

  if (melon_init(0))
    return 1;

  melon_deinit();

  return 0;
}
