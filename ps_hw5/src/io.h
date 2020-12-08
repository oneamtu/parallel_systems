#pragma once

#include "argparse.h"
#include "particle.h"
#include <iostream>
#include <fstream>

void read_file(const struct options_t *args,
    int *n_particles,
    particle **particles);

void write_file(const struct options_t *args,
    int n_particles,
    particle *particles);
