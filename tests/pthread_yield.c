#include <pthread.h>

int main()
{
  while (1)
    pthread_yield();
  return 0;
}
