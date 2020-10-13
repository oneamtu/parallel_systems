#pragma once

#include <iostream>
#include <cstring>
#include <stdlib.h>

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define DEBUG_OUT(x) do { if (DEBUG_TEST) { std::cerr << "DEBUG: " << (x) << std::endl;} } while (0)
#define DEBUG_PRINT(x) do { if (DEBUG_TEST) { printf("DEBUG: "); x;} } while (0)

#define POW2(x) (x)*(x)

#define REAL_FLOAT 1

#ifdef REAL_FLOAT
using real = float;
#else
using real = double;
#endif

#define PRINT_POINT(x, d, i) { \
  for (int _i = i*d; _i < (i+1)*d; _i++) \
    printf("%lf ", (x)[_i]); \
}

#define PRINT_CENTROIDS(centroids, d, k) { \
  for (int cluster_id = 0; cluster_id < k; cluster_id++) { \
    printf("%d ", cluster_id); \
    PRINT_POINT(centroids, d, cluster_id); \
    printf("\n"); \
  } \
}
