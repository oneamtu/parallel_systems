#pragma once

#include "common.h"
#include "particle.h"

int init_visualization();

void terminate_visualization();

bool render_visualization(int n_particles,
    const struct particle *particles,
    const struct quad_tree *root);
