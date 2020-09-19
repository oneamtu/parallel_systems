#include "pthread_barrier.h"

pthread_barrier_t *alloc_pthread_barrier()
{
  return (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t));
}

void init_pthread_barrier(pthread_barrier_t *barrier,
    int n_threads) {
  int res = 0;

  res = pthread_barrier_init(barrier, NULL, n_threads);

  if (res) {
    std::cerr << "Error init-ing barrier: " << strerror(res) << std::endl;
    exit(1);
  }
}
