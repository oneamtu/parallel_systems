#pragma once

#include <iostream>
#include <cstring>
#include <stdlib.h>

#ifdef DEBUG
#undef DEBUG
#define DEBUG(x) do { std::cerr << "DEBUG: " << (x) << std::endl; } while (0)
#define DEBUG_PRINT(x) do { printf("DEBUG: "); x; } while (0)
#else
#define DEBUG(x)
#define DEBUG_PRINT(x)
#endif //DEBUG

#define POW2(x) (x)*(x)

using real = double;

#define PRINT_POINT(x, d, i) { \
  for (int _i = i*d; _i < (i+1)*d; _i++) \
    printf("%lf ", x[_i]); \
}

#define PRINT_CENTROIDS(centroids, d, k) { \
  for (int cluster_id = 0; cluster_id < k; cluster_id++) { \
    printf("%d ", cluster_id); \
    PRINT_POINT(centroids, d, cluster_id); \
    printf("\n"); \
  } \
}
