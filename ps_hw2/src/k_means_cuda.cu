#include <iostream>

#include "k_means_cuda.h"
#include "common.h"

#include <cuda_runtime.h>
#include "ext/helper_cuda.h"

int k_means_cuda(int n_points, real *points, struct options_t *opts,
  int* point_cluster_ids, real** centroids) {

  int device_id = gpuGetMaxGflopsDeviceId();

  cudaDeviceProp deviceProp;
  checkCudaErrors(cudaGetDeviceProperties(&deviceProp, device_id));
  checkCudaErrors(cudaSetDevice(device_id));
  DEBUG_PRINT(printf("gpuDeviceInit() CUDA Device [%d]: \"%s\n", device_id, deviceProp.name));

  bool done = false;
  int iterations = 0;
  int k = opts->n_clusters;
  int d = opts->dimensions;

  cudaEvent_t start, stop;

  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  real *d_points;
  checkCudaErrors(cudaMalloc(&d_points, n_points * d * sizeof(real)));

  real *old_centroids;
  checkCudaErrors(cudaMalloc(&old_centroids, k * d * sizeof(real)));

  real *new_centroids;
  checkCudaErrors(cudaMalloc(&new_centroids, k * d * sizeof(real)));

  real *point_centroid_distances;
  checkCudaErrors(cudaMalloc(&point_centroid_distances, n_points * k * sizeof(real)));

  int *d_pointcluster_ids;
  checkCudaErrors(cudaMalloc(&point_centroid_distances, n_points * sizeof(int)));
  // dv_int unsorted_d_point_cluster_ids(n_points);

  int *d_k_counts;
  checkCudaErrors(cudaMalloc(&point_centroid_distances, k * sizeof(int)));

  // copy data
  // timer code - 0_Simple/simpleMultiCopy/simpleMultiCopy.cu
  cudaEventRecord(start, 0);
  checkCudaErrors(cudaMemcpyAsync(d_points, points, n_points * d * sizeof(real),
                                  cudaMemcpyHostToDevice, 0));

  checkCudaErrors(cudaMemcpyAsync(old_centroids, *centroids, k * d * sizeof(real),
                                  cudaMemcpyHostToDevice, 0));
  cudaEventRecord(stop,0);
  cudaEventSynchronize(stop);

  float memcpy_h2d_time;
  cudaEventElapsedTime(&memcpy_h2d_time, start, stop);
  DEBUG_PRINT(printf("Device to host: %f ms \n", memcpy_h2d_time));


  // while(!done) {
  //   DEBUG_OUT("old centroid");
  //   D_PRINT_ALL(old_centroids);

  //   distance_component distance_component(d, k,
  //       raw_pointer_cast(d_points.data()), raw_pointer_cast(old_centroids.data()));

  //   // compute the distances for each point/centroid
  //   // https://github.com/NVIDIA/thrust/blob/1d067bdba7aaca4b53cd4d43b98ac180a0308446/examples/sum_rows.cu
  //   reduce_by_key(
  //     make_transform_iterator(counting_iterator<int>(0), _1 / d),
  //     make_transform_iterator(counting_iterator<int>(n_points * k * d), _1 / d),
  //     make_transform_iterator(counting_iterator<int>(0),  distance_component),
  //     make_discard_iterator(), //discard keys
  //     point_centroid_distances.begin()
  //   );

  //   DEBUG_OUT("centroid distance");
  //   D_PRINT_ALL(point_centroid_distances);

  //   // reduce to the index of the nearest centroid
  //   reduce_by_key(
  //     make_transform_iterator(counting_iterator<int>(0), _1 / k),
  //     make_transform_iterator(counting_iterator<int>(n_points * k), _1 / k),
  //     make_zip_iterator(
  //       make_tuple(
  //         point_centroid_distances.begin(),
  //         counting_iterator<int>(0))),
  //     make_discard_iterator(),
  //     make_zip_iterator(
  //       make_tuple(
  //         make_discard_iterator(), //discard distance values
  //         unsorted_d_point_cluster_ids.begin())),
  //     equal_to<int>(),
  //     maximum_by_first()
  //   );

  //   transform(
  //     unsorted_d_point_cluster_ids.begin(),
  //     unsorted_d_point_cluster_ids.end(),
  //     unsorted_d_point_cluster_ids.begin(),
  //     _1 % k
  //   );

  //   DEBUG_OUT("%k unsorted_d_point_cluster_ids");
  //   D_PRINT_ALL(unsorted_d_point_cluster_ids);

  //   // zero the new centroids
  //   fill(new_centroids.begin(), new_centroids.end(), 0);

  //   // point_component_centroid_id point_component_centroid_id(d,
  //   //     raw_pointer_cast(d_point_cluster_ids.data()),
  //   //     raw_pointer_cast(new_centroids.data()));
  //   // dv_int v(k*d);

  //   //compute the sum of points for each centroid
  //   // reduce_by_key(
  //   //   make_transform_iterator(counting_iterator<int>(0), point_component_centroid_id),
  //   //   make_transform_iterator(counting_iterator<int>(n_points * d), point_component_centroid_id),
  //   //   d_points.begin(),
  //   //   v.begin(),
  //   //   // make_discard_iterator(), //discard keys
  //   //   new_centroids.begin()
  //   // );
  //   // ok, no fancy reduce_by_key

  //   centroid_means centroid_means(d,
  //     raw_pointer_cast(unsorted_d_point_cluster_ids.data()),
  //     raw_pointer_cast(new_centroids.data()));

  //   for_each(
  //     make_zip_iterator(
  //       make_tuple(d_points.begin(), counting_iterator<int>(0))),
  //     make_zip_iterator(
  //       make_tuple(d_points.end(), counting_iterator<int>(n_points * d))),
  //     centroid_means
  //   );

  //   DEBUG_OUT("new_centroids sums");
  //   D_PRINT_ALL(new_centroids);
  //   // D_PRINT_ALL(v);

  //   d_point_cluster_ids = unsorted_d_point_cluster_ids;

  //   sort(
  //     d_point_cluster_ids.begin(),
  //     d_point_cluster_ids.end()
  //   );

  //   //compute the point count for each centroid
  //   auto new_end = reduce_by_key(
  //     d_point_cluster_ids.begin(),
  //     d_point_cluster_ids.end(),
  //     make_constant_iterator(1),
  //     d_k_count_keys.begin(),
  //     d_k_counts.begin()
  //   );

  //   // zero out any stragglers
  //   fill(new_end.second, d_k_counts.end(), 0);

  //   DEBUG_OUT("k_counts");
  //   D_PRINT_ALL(d_k_counts);
  //   D_PRINT_ALL(d_k_count_keys);
  //   // assert(new_end.first == d_k_count_keys.end());

  //   for (int i = 0; i < d; i++) {
  //     for_each(
  //       make_zip_iterator(
  //         make_tuple(d_k_count_keys.begin(), d_k_counts.begin())),
  //       make_zip_iterator(
  //         make_tuple(new_end.first, new_end.second)),
  //       centroid_divide(d, i, raw_pointer_cast(new_centroids.data()))
  //     );
  //   }

  //   DEBUG_OUT("new_centroids means");
  //   D_PRINT_ALL(new_centroids);

  //   // swap centroids
  //   swap(new_centroids, old_centroids);

  //   real l1_thresh = opts->threshold/d;

  //   bool converged = transform_reduce(
  //     make_zip_iterator(
  //       make_tuple(new_centroids.begin(), old_centroids.begin())),
  //     make_zip_iterator(
  //       make_tuple(new_centroids.end(), old_centroids.end())),
  //     l1_op(l1_thresh),
  //     true,
  //     logical_and<bool>()
  //   );

  //   iterations++;
  //   done = (iterations > opts->max_iterations) || converged;
  // }
  // // release the other centroids buffer

  // DEBUG_OUT(iterations > opts->max_iterations ? "Max iterations reached!" : "Converged!" );

  // copy(old_centroids.begin(), old_centroids.end(), *centroids);
  // copy(unsorted_d_point_cluster_ids.begin(), unsorted_d_point_cluster_ids.end(), point_cluster_ids);

  // return iterations;
  return 0;
}
