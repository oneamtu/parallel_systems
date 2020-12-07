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

  // Setup args & read input data
  int n_particles;
  particle *particles;
  read_file(&opts, &n_particles, &particles);

  if (opts.visualization) {
    init_visualization(&argc, argv);
  }

  // Start timer
  auto start = std::chrono::high_resolution_clock::now();

  // TODO: diff between chrono and MPI timer?
  if (opts.sequential) {
    DEBUG_OUT("Run sequential");

    barnes_hut_sequential(&opts, n_particles, particles);
  } else {
    DEBUG_OUT("Run MPI");

    barnes_hut_mpi(&opts, n_particles, particles);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "time: " << diff.count() << std::endl;

  write_file(&opts, n_particles, particles);

  if (opts.visualization) {
    terminate_visualization();
  }
}
