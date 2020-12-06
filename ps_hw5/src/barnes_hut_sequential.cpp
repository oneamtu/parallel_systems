#include "barnes_hut_sequential.h"

#include <cmath>
#include <csignal>

#include "visualization.h"

quad_tree *insert_particle_into_quad_tree(quad_tree *node, struct particle *particle,
    double partition_x, double partition_y, double s_x, double s_y) {
  if (node == NULL) {
    quad_tree *external_node = (quad_tree *)malloc(sizeof(quad_tree));

    external_node->p = particle;

    external_node->mass = 0;
    external_node->com_x = 0;
    external_node->com_y = 0;

    external_node->partition_x = partition_x;
    external_node->partition_y = partition_y;

    external_node->s_x = s_x;
    external_node->s_y = s_y;

    external_node->n_particles = 0;

    external_node->nw = NULL;
    external_node->ne = NULL;
    external_node->sw = NULL;
    external_node->se = NULL;

    return external_node;
  } else {
    // internal node
    if (node->p == NULL) {
      node->mass += particle->mass;
      node->com_x += particle->x;
      node->com_y += particle->y;

      node->n_particles += 1;

      if (particle->x <= partition_x && particle->y < partition_y) {
        node->nw = insert_particle_into_quad_tree(
            node->nw, particle, partition_x - s_x/2, partition_y - s_y/2, s_x/2, s_y/2);
      } else if (particle->x > partition_x && particle->y < partition_y) {
        node->ne = insert_particle_into_quad_tree(
            node->ne, particle, partition_x + s_x/2, partition_y - s_y/2, s_x/2, s_y/2);
      } else if (particle->x <= partition_x && particle->y >= partition_y) {
        node->sw = insert_particle_into_quad_tree(
            node->sw, particle, partition_x - s_x/2, partition_y + s_y/2, s_x/2, s_y/2);
      } else if (particle->x > partition_x && particle->y >= partition_y) {
        node->se = insert_particle_into_quad_tree(
            node->se, particle, partition_x + s_x/2, partition_y + s_y/2, s_x/2, s_y/2);
      }

      return node;
    //external node
    } else {
      //make the external node internal, move the particle

      auto particle_to_move = node->p;
      node->p = NULL;

      insert_particle_into_quad_tree(
          node, particle_to_move, partition_x, partition_y, s_x, s_y);
      insert_particle_into_quad_tree(
          node, particle, partition_x, partition_y, s_x, s_y);

      return node;
    }
  }
}

quad_tree *build_quad_tree(int n_particles,
    struct particle *particles) {

  quad_tree *root = NULL;

  for (int i = 0; i < n_particles; i++) {
    if (particles[i].mass == OUT_OF_BOUNDS_MASS) {
      continue ;
    }

    root = insert_particle_into_quad_tree(
        root,
        particles + i,
        (MIN_X + MAX_X)/2,
        (MIN_Y + MAX_Y)/2,
        (MAX_X - MIN_X)/2,
        (MAX_Y - MIN_Y)/2);
  }

  return root;
}

void free_quad_tree(struct quad_tree *node) {
  if (node == NULL)
    return ;

  free_quad_tree(node->nw);
  free_quad_tree(node->ne);
  free_quad_tree(node->sw);
  free_quad_tree(node->se);

  free(node);
}

inline double dist_2(const particle *p, const quad_tree *node) {
  return (POW2(node->com_x/node->n_particles - p->x)
      + POW2(node->com_y/node->n_particles - p->y));
}

void compute_force(const struct particle *p,
    const struct quad_tree *node, double theta, double *a_x, double *a_y) {
  if (node == NULL) {
    return ;
  } else if (node->p != NULL) {
    if (node->p != p && node->p->mass != OUT_OF_BOUNDS_MASS) {
      // external node, compute
      auto dx = node->p->x - p->x;
      auto dy = node->p->y - p->y;
      auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);
      auto d3 = d*d*d;

      *a_x += (G*node->p->mass*dx)/d3;
      *a_y += (G*node->p->mass*dy)/d3;
    }
  } else if (POW2(node->s_x) < dist_2(p, node) * theta) {
    // ratio under theta, approximate
    auto dx = node->com_x/node->n_particles - p->x;
    auto dy = node->com_y/node->n_particles - p->y;
    auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);
    auto d3 = d*d*d;

    *a_x += (G*(node->mass/node->n_particles)*dx)/d3;
    *a_y += (G*(node->mass/node->n_particles)*dy)/d3;
  } else {
    // ratio over theta, recurse
    compute_force(p, node->nw, theta, a_x, a_y);
    compute_force(p, node->ne, theta, a_x, a_y);
    compute_force(p, node->sw, theta, a_x, a_y);
    compute_force(p, node->se, theta, a_x, a_y);
  }
}

void update_forces(const struct quad_tree *root,
    const struct quad_tree *node, double theta, double d_t) {
  if (node == NULL) {
    return ;
  } else if (node->p != NULL) {
    double a_x = 0.0f, a_y = 0.0f;

    compute_force(node->p, root, theta, &a_x, &a_y);

    node->p->x += node->p->v_x*d_t + 0.5f*a_x*POW2(d_t);
    node->p->y += node->p->v_y*d_t + 0.5f*a_y*POW2(d_t);

    if (node->p->x < MIN_X || node->p->x > MAX_X
        || node->p->y < MIN_Y || node->p->y > MAX_Y) {
      node->p->mass = OUT_OF_BOUNDS_MASS;
    }

    node->p->v_x += a_x*d_t;
    node->p->v_y += a_y*d_t;
  } else {
    update_forces(root, node->nw, theta, d_t);
    update_forces(root, node->ne, theta, d_t);
    update_forces(root, node->sw, theta, d_t);
    update_forces(root, node->se, theta, d_t);
  }
}

void barnes_hut_sequential(const struct options_t *args,
    int n_particles,
    struct particle *particles) {

  for (int i = 0; i < args->steps; i++) {
    DEBUG_PRINT(printf("Step %d\n", i));

    quad_tree *root = build_quad_tree(n_particles, particles);

    if (args->visualization) {
      bool quit = render_visualization(n_particles, particles, root);

      if (quit) {
        return ;
      }
    }

    update_forces(root, root, args->theta, args->delta);
    free_quad_tree(root);
  }
}
