#include "prefix_sum.h"
#include "helpers.h"

void synchronize_on_barrier(prefix_sum_args_t* args) {
  if (args->spin) {
    // TODO
  }
  else {
    pthread_barrier_wait((pthread_barrier_t *)args->barrier);
  }
}

void *compute_prefix_sum(void *a) {
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    // Implementation of Blelloch n/p blocks + p processor sum scan
    // https://www.cs.cmu.edu/afs/cs/academic/class/15750-s11/www/handouts/PrefixSumBlelloch.pdf

    // sum block size for each thread; has to cover all values even for uneven
    // divisions
    int block_size = args->n_vals / args->n_threads +
      (args->n_vals % args->n_threads == 0 ? 0 : 1);

    int t_i_partition_start = block_size * args->t_id;
    int t_i_partition_end = t_i_partition_start + block_size;

    // compute processor sums for each block
    // they'll be found at each t_i_partition_end
    args->output_vals[t_i_partition_start] = args->input_vals[t_i_partition_start];
    for (int i = t_i_partition_start + 1; i < t_i_partition_end && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[i-1], args->input_vals[i], args->n_loops);
    }

    synchronize_on_barrier(args);

    // reduce the partition processor sums
    if (args->t_id == 0) {
        args->output_vals[0] = args->input_vals[0];
        for (int i = 2*block_size - 1; i < args->n_vals; i += block_size) {
            args->output_vals[i] = args->op(args->output_vals[i-block_size], args->output_vals[i], args->n_loops);
        }
    }

    synchronize_on_barrier(args);

    // // incorporate reduced processor sums back into each block
    // // the first block is already processed, and the last one is not so offset
    // // the block count
    for (int i = t_i_partition_end; i < t_i_partition_end + block_size - 1 && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[t_i_partition_end - 1], args->output_vals[i], args->n_loops);
    }

    return 0;
}
