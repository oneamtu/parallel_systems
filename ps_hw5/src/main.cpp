#include <iostream>
#include "argparse.h"
#include "barnes_hut_sequential.h"
#include "barnes_hut_mpi.h"
#include "particle.h"
#include "io.h"
#include "common.h"
#include "visualization.h"
#include <chrono>
#include <cstring>

int main(int argc, char **argv)
{
  // Parse args
  struct options_t opts;
  get_opts(argc, argv, &opts);

  if (opts.visualization) {
    init_visualization(&argc, argv);
  }

  if (opts.sequential) {
    DEBUG_OUT("Run sequential");

    barnes_hut_sequential(&opts);
  } else {
    DEBUG_OUT("Run MPI");

    barnes_hut_mpi(&opts);
  }

  if (opts.visualization) {
    terminate_visualization();
  }
}
