#ifndef _PTHREAD_BARRIER_H
#define _PTHREAD_BARRIER_H

#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

pthread_barrier_t* alloc_pthread_barrier();

void init_pthread_barrier(pthread_barrier_t* barrier,
                  int n_threads);

#endif
