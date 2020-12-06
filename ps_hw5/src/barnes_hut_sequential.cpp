#include "barnes_hut_sequential.h"

#include "visualization.h"

quad_tree *insert_particle_into_quad_tree(quad_tree *node, struct particle *particle,
    double partition_x, double partition_y, double len_x, double len_y) {
  if (node == NULL) {
    quad_tree *external_node = (quad_tree *)malloc(sizeof(quad_tree));

    external_node->p = particle;

    external_node->mass = 0;
    external_node->com_x = 0;
    external_node->com_y = 0;

    external_node->partition_x = partition_x;
    external_node->partition_y = partition_y;

    external_node->n_particles = 0;

    external_node->nw = NULL;
    external_node->ne = NULL;
    external_node->sw = NULL;
    external_node->se = NULL;

    return external_node;
  } else {
    // internal node
    if (node->p == NULL) {
      // use num particles
      node->mass += particle->mass;
      node->com_x += particle->x;
      node->com_y += particle->y;

      node->n_particles += 1;

      if (particle->x <= partition_x && particle->y < partition_y) {
        node->nw = insert_particle_into_quad_tree(
            node->nw, particle, partition_x - len_x/2, partition_y - len_y/2, len_x/2, len_y/2);
      } else if (particle->x > partition_x && particle->y < partition_y) {
        node->ne = insert_particle_into_quad_tree(
            node->ne, particle, partition_x + len_x/2, partition_y - len_y/2, len_x/2, len_y/2);
      } else if (particle->x <= partition_x && particle->y >= partition_y) {
        node->sw = insert_particle_into_quad_tree(
            node->sw, particle, partition_x - len_x/2, partition_y + len_y/2, len_x/2, len_y/2);
      } else if (particle->x > partition_x && particle->y >= partition_y) {
        node->se = insert_particle_into_quad_tree(
            node->se, particle, partition_x + len_x/2, partition_y + len_y/2, len_x/2, len_y/2);
      }

      return node;
    //external node
    } else {
      //make the external node internal, move the particle

      auto particle_to_move = node->p;
      node->p = NULL;

      insert_particle_into_quad_tree(
          node, particle_to_move, partition_x, partition_y, len_x, len_y);
      insert_particle_into_quad_tree(
          node, particle, partition_x, partition_y, len_x, len_y);

      return node;
    }
  }
}

quad_tree *build_quad_tree(int n_particles,
    struct particle *particles) {

  quad_tree *root = NULL;

  for (int i = 0; i < n_particles; i++) {
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

void barnes_hut_sequential(const struct options_t *args,
    int n_particles,
    struct particle *particles) {

  quad_tree *root = build_quad_tree(n_particles, particles);

  if (args->visualization) {
    bool quit = render_visualization(n_particles, particles, root);

    if (quit) {
      return ;
    }
  }
}
