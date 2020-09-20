#include "spin_barrier.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include "helpers.h"

spin_barrier_t *spin_barrier_alloc()
{
  return (spin_barrier_t *)malloc(sizeof(spin_barrier_t));
}

void spin_barrier_init(spin_barrier_t *barrier, int n_threads) {
  HANDLE(pthread_spin_init(&(barrier->arrival), 0));
  HANDLE(pthread_spin_init(&(barrier->departure), 0));

  // Start with departure locked
  HANDLE(pthread_spin_lock(&(barrier->departure)));

  barrier->counter = 0;
  barrier->n_threads = n_threads;
}

void spin_barrier_destroy(spin_barrier_t* barrier) {
  HANDLE(pthread_spin_unlock(&(barrier->departure)));

  HANDLE(pthread_spin_destroy(&(barrier->arrival)));
  HANDLE(pthread_spin_destroy(&(barrier->departure)));
}

void spin_barrier_wait(spin_barrier_t* barrier) {
  HANDLE(pthread_spin_lock(&(barrier->arrival)));

  DEBUG("Arrival barrier counter:");
  DEBUG(barrier->counter);
  if (++barrier->counter < barrier->n_threads) {
    HANDLE(pthread_spin_unlock(&(barrier->arrival)));
  }
  else {
    HANDLE(pthread_spin_unlock(&(barrier->departure)));
  }

  HANDLE(pthread_spin_lock(&(barrier->departure)));

  DEBUG("Departure barrier counter:");
  DEBUG(barrier->counter);
  if (--barrier->counter > 0) {
    HANDLE(pthread_spin_unlock(&(barrier->departure)));
  }
  else {
    HANDLE(pthread_spin_unlock(&(barrier->arrival)));
  }
}
