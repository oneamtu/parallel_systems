#include <iostream>
#include "argparse.h"
#include "threads.h"
#include "pthread_barrier.h"
#include "spin_barrier.h"
#include "io.h"
#include <chrono>
#include <cstring>
#include "operators.h"
#include "helpers.h"
#include "prefix_sum.h"

int main(int argc, char **argv)
{
  // Parse args
  struct options_t opts;
  get_opts(argc, argv, &opts);

  bool sequential = false;
  if (opts.n_threads == 0) {
    opts.n_threads = 1;
    sequential = true;
  }

  // Setup threads
  pthread_t *threads = sequential ? NULL : alloc_threads(opts.n_threads);;

  // Setup args & read input data
  prefix_sum_args_t *ps_args = alloc_args(opts.n_threads);
  int n_vals;
  int *input_vals, *output_vals;
  read_file(&opts, &n_vals, &input_vals, &output_vals);

  //"op" is the operator you have to use, but you can use "add" to test
  int (*scan_operator)(int, int, int);
  scan_operator = op;
  // scan_operator = add;

  DEBUG("init barrier");
  void *barrier;

  if (opts.spin) {
    barrier = (void *) spin_barrier_alloc();
    spin_barrier_init((spin_barrier_t *)barrier, opts.n_threads);
  }
  else {
    barrier = (void *) alloc_pthread_barrier();
    init_pthread_barrier((pthread_barrier_t *)barrier, opts.n_threads);
  }

  fill_args(ps_args,
      input_vals, output_vals,
      opts.spin, (void *)barrier,
      opts.n_threads, n_vals,
      scan_operator, opts.n_loops);

  // Start timer
  auto start = std::chrono::high_resolution_clock::now();

  if (sequential)  {
    DEBUG("Run sequential");
    //sequential prefix scan
    output_vals[0] = input_vals[0];
    for (int i = 1; i < n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      output_vals[i] = scan_operator(output_vals[i-1], input_vals[i], ps_args->n_loops);
    }
  }
  else {
    DEBUG("Run threads");

    switch (opts.algorithm)
    {
      case 0:
        start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_block_sequential_sum);
        break;
      case 1:
        start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_block_parallel_sum);
        break;
      case 2:
        start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_tree_sum);
        break;
    }

    // Wait for threads to finish
    join_threads(threads, opts.n_threads);
  }

  //End timer and print out elapsed
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "time: " << diff.count() << std::endl;

  // Write output data
  write_file(&opts, &(ps_args[0]));

  // Free other buffers
  if (opts.spin) {
    spin_barrier_destroy((spin_barrier_t *)barrier);
    free((spin_barrier_t *)barrier);
  }
  else {
    pthread_barrier_destroy((pthread_barrier_t *)barrier);
    free((pthread_barrier_t *)barrier);
  }
  free(threads);
  free(ps_args);
}
