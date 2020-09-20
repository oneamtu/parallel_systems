#include "prefix_sum.h"
#include "helpers.h"

void synchronize_on_barrier(prefix_sum_args_t* args) {
  if (args->spin) {
    spin_barrier_wait((spin_barrier_t *)args->barrier);
  }
  else {
    pthread_barrier_wait((pthread_barrier_t *)args->barrier);
  }
}

// Implementation of parallel tree sum reduce/scan
// https://www.cs.cmu.edu/afs/cs/academic/class/15750-s11/www/handouts/PrefixSumBlelloch.pdf
void *compute_prefix_parallel_tree_sum(void *a) {
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    // tree lg(p) implementation of the processor scan
    // reduce/up-sweep sums
    int max_offset = 0;
    for (int offset = 1; offset < args->n_vals; offset <<= 1) {
      max_offset = offset;

      int step_size = offset << 1;
      // partition size for a thread
      int block_size = args->n_vals / (step_size * args->n_threads) +
        (args->n_vals % (step_size * args->n_threads) == 0 ? 0 : 1);
      // std::cerr << block_size << std::endl;
      block_size = (block_size == 0 ? 1 : block_size);

      for (int i = args->t_id * block_size * step_size;
          i < (args->t_id * block_size + block_size) * step_size && i < args->n_vals; i += step_size) {
        int dest_index = (i + step_size) - 1;
        int prev_index = (i + offset) - 1;

        // std::cerr << args->t_id << " " << i << " " << dest_index << " " << prev_index << " " << std::endl;

        if (offset == 1) {
          args->output_vals[prev_index] = args->input_vals[prev_index];
          if (dest_index < args->n_vals) {
            args->output_vals[dest_index] = args->input_vals[dest_index];
          }
        }

        if (dest_index < args->n_vals) {
          args->output_vals[dest_index] =
            args->op(args->output_vals[dest_index], args->output_vals[prev_index], args->n_loops);
        }
      }

      synchronize_on_barrier(args);
    }

    // scan/down-sweep sums
    for (int offset = max_offset; offset > 0; offset >>= 1) {
      // for (int i = args->t_id * offset; i < args->t_id * (offset + 1) && i < args->n_threads; i += offset) {
      int step_size = offset << 1;
      // partition size for a thread
      int block_size = args->n_vals / (step_size * args->n_threads) +
        (args->n_vals % (step_size * args->n_threads) == 0 ? 0 : 1);
      block_size = (block_size == 0 ? 1 : block_size);

      for (int i = args->t_id * block_size * step_size;
          i < (args->t_id * block_size + block_size) * step_size && i < args->n_vals; i += step_size) {
        int reduced_index = (i + step_size) - 1;
        int dest_index = (i + step_size + offset) - 1;

        if (dest_index < args->n_vals) {
          args->output_vals[dest_index] =
            args->op(args->output_vals[dest_index], args->output_vals[reduced_index], args->n_loops);
        }
      }

      synchronize_on_barrier(args);
    }

    return 0;
}

// Implementation of n/p blocks + parallel p processor sum reduce/scan
// https://www.cs.cmu.edu/afs/cs/academic/class/15750-s11/www/handouts/PrefixSumBlelloch.pdf
void *compute_prefix_parallel_block_parallel_sum(void *a) {
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    // sum block size for each thread; has to cover all values even for uneven
    // divisions
    int block_size = args->n_vals / args->n_threads +
      (args->n_vals % args->n_threads == 0 ? 0 : 1);

    int t_i_partition_start = block_size * args->t_id;
    int t_i_partition_end = t_i_partition_start + block_size;

    // compute processor sums for each block
    // they'll be found at each t_i_partition_end
    if (t_i_partition_start < args->n_vals) {
      args->output_vals[t_i_partition_start] = args->input_vals[t_i_partition_start];
    }
    for (int i = t_i_partition_start + 1; i < t_i_partition_end && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[i-1], args->input_vals[i], args->n_loops);
    }

    synchronize_on_barrier(args);

    // if (args->t_id == 0) {
    //     for (int i = block_size - 1; i < args->n_vals; i += block_size) {
    //       std::cerr << args->output_vals[i] << " ";
    //     }
    //     std::cerr << std::endl;
    // }

    // tree lg(p) implementation of the processor scan
    // reduce/up-sweep sums
    int max_offset = 0;
    for (int offset = 1; offset < args->n_threads; offset <<= 1) {
      // for (int i = args->t_id * offset; i < args->t_id * (offset + 1) && i < args->n_threads; i += offset) {
      int step_size = offset << 1;
      max_offset = offset;
      int i = args->t_id * step_size;

      if (i < args->n_threads) {
        int dest_block_sum_index = (i + step_size) * block_size - 1;
        int prev_block_sum_index = (i + offset) * block_size - 1;

        if (dest_block_sum_index < args->n_vals) {
          args->output_vals[dest_block_sum_index] =
            args->op(args->output_vals[dest_block_sum_index], args->output_vals[prev_block_sum_index], args->n_loops);
        }
      }

      synchronize_on_barrier(args);
    }

    // if (args->t_id == 0) {
    //     for (int i = block_size - 1; i < args->n_vals; i += block_size) {
    //       std::cerr << args->output_vals[i] << " ";
    //     }
    //     std::cerr << std::endl;
    // }

    // scan/down-sweep sums
    for (int offset = max_offset; offset > 0; offset >>= 1) {
      // for (int i = args->t_id * offset; i < args->t_id * (offset + 1) && i < args->n_threads; i += offset) {
      int step_size = offset << 1;
      int i = args->t_id * step_size;

      if (i < args->n_threads) {
        int reduced_block_sum_index = (i + step_size) * block_size - 1;
        int dest_block_sum_index = (i + step_size + offset) * block_size - 1;

        if (dest_block_sum_index < args->n_vals) {
          args->output_vals[dest_block_sum_index] =
            args->op(args->output_vals[dest_block_sum_index], args->output_vals[reduced_block_sum_index], args->n_loops);
        }
      }

      synchronize_on_barrier(args);
    }

    // if (args->t_id == 0) {
    //     for (int i = block_size - 1; i < args->n_vals; i += block_size) {
    //       std::cerr << args->output_vals[i] << " ";
    //     }
    //     std::cerr << std::endl;
    // }

    // incorporate reduced processor sums back into each block
    // the first block is already processed, and the last one is not so offset
    // the block count
    for (int i = t_i_partition_end; i < t_i_partition_end + block_size - 1 && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[t_i_partition_end - 1], args->output_vals[i], args->n_loops);
    }

    return 0;
}

// Implementation of n/p blocks + sequential p processor sum reduce/scan
// https://www.cs.cmu.edu/afs/cs/academic/class/15750-s11/www/handouts/PrefixSumBlelloch.pdf
void *compute_prefix_parallel_block_sequential_sum(void *a) {
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

    // sum block size for each thread; has to cover all values even for uneven
    // divisions
    int block_size = args->n_vals / args->n_threads +
      (args->n_vals % args->n_threads == 0 ? 0 : 1);

    int t_i_partition_start = block_size * args->t_id;
    int t_i_partition_end = t_i_partition_start + block_size;

    // compute processor sums for each block
    // they'll be found at each t_i_partition_end

    if (t_i_partition_start < args->n_vals) {
      args->output_vals[t_i_partition_start] = args->input_vals[t_i_partition_start];
    }
    for (int i = t_i_partition_start + 1; i < t_i_partition_end && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[i-1], args->input_vals[i], args->n_loops);
    }

    synchronize_on_barrier(args);

    // Sequential reduce/scan on the block sums
    if (args->t_id == 0) {
      for (int i = 2*block_size - 1; i < args->n_vals; i += block_size) {
        args->output_vals[i] = args->op(args->output_vals[i-block_size], args->output_vals[i], args->n_loops);
      }
    }

    synchronize_on_barrier(args);

    // incorporate reduced processor sums back into each block
    // the first block is already processed, and the last one is not so offset
    // the block count
    for (int i = t_i_partition_end; i < t_i_partition_end + block_size - 1 && i < args->n_vals; ++i) {
      //y_i = y_{i-1}  <op>  x_i
      args->output_vals[i] = args->op(args->output_vals[t_i_partition_end - 1], args->output_vals[i], args->n_loops);
    }

    return 0;
}
