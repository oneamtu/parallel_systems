#pragma once

#include "particle.h"
#include "common.h"
#include "argparse.h"

quad_tree *insert_particle_into_quad_tree(quad_tree *node, struct particle *particle,
    double partition_x, double partition_y, double s_x, double s_y);

quad_tree *build_quad_tree(int n_particles,
    struct particle *particles);

void free_quad_tree(struct quad_tree *node);

void update_force(struct particle *p,
    const struct quad_tree *node, double theta);

void update_velocity_position(particle *p, double d_t);

void barnes_hut_sequential(const struct options_t *args);
