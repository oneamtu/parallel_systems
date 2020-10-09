#include <iostream>
#include "argparse.h"
#include "io.h"
#include "debug.h"
#include <chrono>
#include <cstring>
#include <cfloat>

#define POW2(x) (x)*(x)

static unsigned long int next = 1;
static unsigned long kmeans_rmax = 32767;
int k_means_rand() {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % (kmeans_rmax+1);
}
void k_means_srand(unsigned int seed) {
    next = seed;
}

void k_means_init_random_centroids(int n_points, int d, double *points,
    int k, double *centroids, int seed) {
  k_means_srand(seed); // cmd_seed is a cmdline arg

  for (int i = 0; i < k; i++) {
    int index = k_means_rand() % n_points;
    std::memcpy(&centroids[i * d], &points[index * d], d * sizeof(double));
  }
}

void assign_point_cluster_ids(int n_points, int d, double *points,
    int *point_cluster_ids, int k, int *k_counts, double *centroids) {

  std::memset(k_counts, 0, sizeof(int) * k);

  for (int i = 0; i < n_points; i++) {
    int nearest_centroid = -1;
    double nearest_centroid_dist = DBL_MAX;

    for (int j = 0; j < k; j++) {
      double dist_2 = 0;

      for (int l = 0; l < d; l++) {
        dist_2 += POW2(centroids[j*d + l] - points[i*d + l]);
      }

      if (nearest_centroid_dist > dist_2) {
        nearest_centroid_dist = dist_2;
        nearest_centroid = j;
      }
    }

    point_cluster_ids[i] = nearest_centroid;
    k_counts[nearest_centroid]++;
  }
}

void compute_new_centroids(int n_points, int d, double *points,
    int *point_cluster_ids, int k, int *k_counts, double *centroids) {
  std::memset(centroids, 0, sizeof(double) * d * k);

  for (int i = 0; i < n_points; i++) {
    for (int l = 0; l < d; l++) {
      centroids[point_cluster_ids[i]*d + l] += points[i*d + l] / k_counts[point_cluster_ids[i]];
    }
  }
}

bool converged(int k, int d, double *centroids_1, double *centroids_2) {
  return false;
}

void k_means_sequential(int n_points, double *points, struct options_t *opts,
    int** point_cluster_ids, double** centroids) {

  double *centroids_1 = (double *)malloc(opts->n_clusters * opts->dimensions * sizeof(double));
  double *centroids_2 = (double *)malloc(opts->n_clusters * opts->dimensions * sizeof(double));
  *point_cluster_ids = (int *)malloc(n_points * sizeof(int));
  int *k_counts = (int *)malloc(opts->n_clusters * sizeof(int));

  k_means_init_random_centroids(n_points, opts->dimensions, points,
      opts->n_clusters, centroids_1, opts->seed);

  // TODO
  bool done = false;
  int iterations = 0;
  double **old_centroids = &centroids_1;
  double **new_centroids = &centroids_2;

  while(!done) {
    assign_point_cluster_ids(n_points, opts->dimensions, points,
        *point_cluster_ids, opts->n_clusters, k_counts, *old_centroids);

    compute_new_centroids(n_points, opts->dimensions, points,
        *point_cluster_ids, opts->n_clusters, k_counts, *new_centroids);

    // swap centroids
    *centroids = *new_centroids;
    *new_centroids = *old_centroids;
    *old_centroids = *centroids;

    iterations++;
    done = (iterations > opts->max_iterations) ||
      converged(opts->n_clusters, opts->dimensions, centroids_1, centroids_2);
  }
}

int main(int argc, char **argv) {
  // Parse args
  struct options_t opts;
  get_opts(argc, argv, &opts);

  int n_points;
  double *points;
  read_file(&opts, &n_points, &points);

  int *point_cluster_ids;
  double *centroids;

  // Start timer
  auto start = std::chrono::high_resolution_clock::now();

  switch (opts.algorithm)
  {
    case 0:
      DEBUG("Sequential:");

      k_means_sequential(n_points, points, &opts, &point_cluster_ids, &centroids);
      break;
    case 1:
      // start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_block_parallel_sum);
      break;
    case 2:
      // start_threads(threads, opts.n_threads, ps_args, compute_prefix_parallel_tree_sum);
      break;
  }

  //End timer and print out elapsed
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "time: " << diff.count() << std::endl;

  if (opts.print_centroids) {
    for (int cluster_id = 0; cluster_id < opts.n_clusters; cluster_id++) {
      printf("%d ", cluster_id);
      for (int d = 0; d < opts.dimensions; d++)
        printf("%lf ", centroids[cluster_id + d * opts.n_clusters]);
      printf("\n");
    }
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
