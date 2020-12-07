#pragma once

#include "particle.h"
#include "common.h"
#include "argparse.h"

void barnes_hut_mpi(const struct options_t *args,
    int n_particles,
    particle *particles);
