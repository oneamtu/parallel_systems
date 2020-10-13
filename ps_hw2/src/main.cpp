#include <chrono>
#include <cfloat>
#include "argparse.h"
#include "io.h"
#include "common.h"
#include "seed.h"
#include "k_means_sequential.h"
#include "k_means_thrust.h"

int main(int argc, char **argv) {
  // Parse args
  struct options_t opts;
  get_opts(argc, argv, &opts);

  int n_points;
  real *points;
  read_file(&opts, &n_points, &points);

  int *point_cluster_ids = (int *)malloc(n_points * sizeof(int));
  real *centroids = (real *)malloc(opts.n_clusters * opts.dimensions * sizeof(real));
  k_means_init_random_centroids(n_points, opts.dimensions, points,
      opts.n_clusters, centroids, opts.seed);

  int iterations = 0;

  // Start timer
  auto start = std::chrono::high_resolution_clock::now();

  switch (opts.algorithm)
  {
    case 0:
      DEBUG("Running k_means_sequential:");

      iterations = k_means_sequential(n_points, points, &opts, point_cluster_ids, &centroids);
      break;
    case 1:
      DEBUG("Running k_means_thrust:");

      iterations = k_means_thrust(n_points, points, &opts, point_cluster_ids, &centroids);
      break;
    case 2:
      // start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_tree_sum);
      break;
  }

  //End timer and print out elapsed
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration<double, std::milli>(end - start);
  printf("%d,%lf\n", iterations, diff.count() / iterations);

  if (opts.print_centroids) {
    PRINT_CENTROIDS(centroids, opts.dimensions, opts.n_clusters);
  }
  else {
    printf("clusters:");
    for (int i = 0; i < n_points; i++)
      printf(" %d", point_cluster_ids[i]);
  }

  free(centroids);
  free(point_cluster_ids);
  free(points);
}
