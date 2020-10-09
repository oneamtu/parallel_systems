#include "io.h"
#include "debug.h"

void read_file(struct options_t* args, int* n_points, double** points) {

  // Open file
  std::ifstream in;
  in.open(args->in_file);
  // Get num vals
  in >> *n_points;

  // Alloc input and output arrays
  *points = (double*) malloc(*n_points * args->dimensions * sizeof(double));

  int index;

  // Read input vals
  for (int i = 0; i < (*n_points) * args->dimensions; ++i) {
    in >> index;
    in >> (*points)[i];
  }

  DEBUG(index);
  DEBUG((*points)[0]);
  DEBUG((*points)[*n_points * args->dimensions - 1]);
}
