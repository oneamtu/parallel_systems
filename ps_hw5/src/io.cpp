#include <cstdio>
#include "io.h"
#include "common.h"

void read_file(struct options_t* args,
    int *n_particles,
    particle **particles) {

  FILE *input_f;
  input_f = fopen(args->in_file, "r");

  int scanned;
  scanned = fscanf(input_f, "%d", n_particles);

  if (scanned != 1) {
    std::cerr << "Error reading num particles! Err info: " << scanned << std::endl;
  }

  DEBUG_PRINT(printf("%d particles!\n", *n_particles));

  *particles = (particle *) malloc(*n_particles * sizeof(particle));

  // Read input vals
  for (int i = 0; i < *n_particles; ++i) {
    scanned = fscanf(input_f, "%d\t%lf\t%lf\t%lf\t%lf\t%lf",
        &((*particles)[i].index),
        &((*particles)[i].x),
        &((*particles)[i].y),
        &((*particles)[i].mass),
        &((*particles)[i].v_x),
        &((*particles)[i].v_y));

    if (scanned != 6) {
      std::cerr << "Error reading particle! Err info: " << scanned << std::endl;
    }
  }
  fclose(input_f);
}

void write_file(struct options_t* args,
    int n_particles,
    particle *particles) {

  FILE *output_f;
  output_f = fopen(args->out_file, "w");
  fprintf(output_f, "%d\n", n_particles);

  // Read input vals
  for (int i = 0; i < n_particles; ++i) {
    fprintf(output_f, "%d\t%lf\t%lf\t%lf\t%lf\t%lf\n",
        particles[i].index, particles[i].x, particles[i].y,
        particles[i].mass, particles[i].v_x, particles[i].v_y);
  }
  fclose(output_f);

  // Free memory
  free(particles);
}
