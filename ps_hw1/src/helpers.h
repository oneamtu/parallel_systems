#pragma once

#include "operators.h"
#include <stdlib.h>
#include <pthread.h>
#include "spin_barrier.h"

#define HANDLE(x) \
  do { \
    int res = x; \
    if (res) {\
      std::cerr << __FILE__ << "Error init-ing spin barrier departure spin: " << strerror(res) << std::endl;\
      std::cerr << "[" << __FILE__ << "][" << __FUNCTION__ << "][Line " << __LINE__ << "] " << #x << strerror(res) << std::endl;\
      exit(1);\
    }\
  } while (0) \

#if EBUG
#define DEBUG(x) do { std::cerr << x << std::endl; } while (0)
#else
#define DEBUG(x)
#endif //DEBUG

struct prefix_sum_args_t {
  int*               input_vals;
  int*               output_vals;
  bool               spin;
  void*              barrier;
  int                n_vals;
  int                n_threads;
  int                t_id;
  int (*op)(int, int, int);
  int n_loops;
};

prefix_sum_args_t* alloc_args(int n_threads);

int next_power_of_two(int x);

void fill_args(prefix_sum_args_t *args,
               int *inputs,
               int *outputs,
               bool spin,
               void* barrier,
               int n_threads,
               int n_vals,
               int (*op)(int, int, int),
               int n_loops);
