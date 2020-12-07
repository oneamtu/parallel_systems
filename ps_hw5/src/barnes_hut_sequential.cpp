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

void update_force(struct particle *p,
    const struct quad_tree *node, double theta) {
  if (node == NULL) {
    return ;
  } else if (node->p != NULL) {
    if (node->p != p && node->p->mass != OUT_OF_BOUNDS_MASS) {
      // external node, compute
      auto dx = node->p->x - p->x;
      auto dy = node->p->y - p->y;
      auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);
      auto d3 = d*d*d;

      p->a_x += (G*node->p->mass*dx)/d3;
      p->a_y += (G*node->p->mass*dy)/d3;
    }
  } else {
    auto dx = node->com_x/node->n_particles - p->x;
    auto dy = node->com_y/node->n_particles - p->y;
    auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);

    if ((node->s_x*2)/d < theta) {
      // ratio under theta, approximate
      auto d3 = d*d*d;

      p->a_x += (G*(node->mass/node->n_particles)*dx)/d3;
      p->a_y += (G*(node->mass/node->n_particles)*dy)/d3;
    } else {
      // ratio over theta, recurse
      update_force(p, node->nw, theta);
      update_force(p, node->ne, theta);
      update_force(p, node->sw, theta);
      update_force(p, node->se, theta);
    }
  }
}

void update_velocity_position(particle *p, double d_t) {
    p->x += p->v_x*d_t + 0.5f*p->a_x*POW2(d_t);
    p->y += p->v_y*d_t + 0.5f*p->a_y*POW2(d_t);

    if (p->x < MIN_X || p->x > MAX_X
        || p->y < MIN_Y || p->y > MAX_Y) {
      p->mass = OUT_OF_BOUNDS_MASS;
    }

    p->v_x += p->a_x*d_t;
    p->v_y += p->a_y*d_t;
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

    for (int i = 0; i < n_particles; i++) {
      particles[i].a_x = 0.0f;
      particles[i].a_y = 0.0f;

      update_force(particles + i, root, args->theta);
    }

    for (int i = 0; i < n_particles; i++) {
      update_velocity_position(particles + i, args->delta);
    }

    free_quad_tree(root);
  }
}
