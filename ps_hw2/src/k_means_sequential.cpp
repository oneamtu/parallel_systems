#include <cstring>
#include <cfloat>
#include "common.h"
#include "k_means_sequential.h"
#include "argparse.h"
#include "seed.h"

void assign_point_cluster_ids(int n_points, int d, real *points,
    int *point_cluster_ids, int k, int *k_counts, real *centroids) {

  std::memset(k_counts, 0, sizeof(int) * k);

  for (int i = 0; i < n_points; i++) {
    int nearest_centroid = -1;
    real nearest_centroid_dist = DBL_MAX;

    for (int j = 0; j < k; j++) {
      real dist_2 = 0;

      for (int l = 0; l < d; l++) {
        dist_2 += POW2(centroids[j*d + l] - points[i*d + l]);
      }

      if (nearest_centroid_dist > dist_2) {
        nearest_centroid_dist = dist_2;
        nearest_centroid = j;
      }
    }

    point_cluster_ids[i] = nearest_centroid;
    DEBUG_PRINT(printf("%d %lf ", nearest_centroid, nearest_centroid_dist));
    k_counts[nearest_centroid]++;
  }
}

void compute_new_centroids(int n_points, int d, real *points,
    int *point_cluster_ids, int k, int *k_counts, real *new_centroids, real *old_centroids) {
  std::memset(new_centroids, 0, sizeof(real) * d * k);

  for (int i = 0; i < n_points; i++) {
    for (int l = 0; l < d; l++) {
      new_centroids[point_cluster_ids[i]*d + l] += points[i*d + l];
    }
  }
  for (int i = 0; i < k; i++) {
    if (k_counts[i] == 0) {
      // if the centroid "vanished"
      // pick another random point from the biggest cluster
      int max = 0;
      int max_i = 0;

      for (int l = 0; l < k; l++) {
        if (max < k_counts[i]) {
          max = k_counts[i];
          max_i = i;
        }
      }

      int index;
      { index = k_means_rand() % n_points; } while(point_cluster_ids[index] != max_i);
      std::memcpy(&new_centroids[i*d], &points[index*d], d * sizeof(real));
    }
    else {
      for (int l = 0; l < d; l++) {
        new_centroids[i*d + l] /= k_counts[i];
      }
    }
  }
}

bool converged(int k, int d, real t, real *centroids_1, real *centroids_2) {
  bool result = true;

  real l1_thresh = t/d;
  for (int i = 0; i < k * d; i++) {
    // l1-norm
    result = result && (abs(centroids_1[i] - centroids_2[i]) < l1_thresh);
  }

  return result;
}

int k_means_sequential(int n_points, real *points, struct options_t *opts,
    int* point_cluster_ids, real** centroids) {

  real *centroids_1 = *centroids;
  real *centroids_2 = (real *)malloc(opts->n_clusters * opts->dimensions * sizeof(real));
  int *k_counts = (int *)malloc(opts->n_clusters * sizeof(int));

  bool done = false;
  int iterations = 0;
  real **old_centroids = &centroids_1;
  real **new_centroids = &centroids_2;

  while(!done) {
    printf("Old centroids\n");
    DEBUG(*old_centroids[0]);
    DEBUG(*old_centroids[1]);
    DEBUG(*old_centroids[4]);
    // DEBUG_PRINT(printf("New centroids\n"));
    // DEBUG_PRINT(print_centroids(*new_centroids, opts->dimensions, opts->n_clusters));

    assign_point_cluster_ids(n_points, opts->dimensions, points,
        point_cluster_ids, opts->n_clusters, k_counts, *old_centroids);

    compute_new_centroids(n_points, opts->dimensions, points,
        point_cluster_ids, opts->n_clusters, k_counts, *new_centroids, *old_centroids);

    printf("New new_centroids\n");
    DEBUG(*new_centroids[0]);
    DEBUG(*new_centroids[1]);
    DEBUG(*new_centroids[4]);

    // swap centroids
    *centroids = *new_centroids;
    *new_centroids = *old_centroids;
    *old_centroids = *centroids;

    iterations++;
    done = (iterations > opts->max_iterations) ||
      converged(opts->n_clusters, opts->dimensions, opts->threshold, centroids_1, centroids_2);
  }
  // release the other centroids buffer
  free(*new_centroids);
  free(k_counts);

  DEBUG(iterations > opts->max_iterations ? "Max iterations reached!" : "Converged!" );

  return iterations;
}


