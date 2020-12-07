#include "barnes_hut_mpi.h"

#include <cmath>
#include <csignal>
#include <mpi.h>

#include "barnes_hut_sequential.h"
#include "visualization.h"

// void update_force(struct particle *p,
//     const struct quad_tree *node, double theta) {
//   if (node == NULL) {
//     return ;
//   } else if (node->p != NULL) {
//     if (node->p != p && node->p->mass != OUT_OF_BOUNDS_MASS) {
//       // external node, compute
//       auto dx = node->p->x - p->x;
//       auto dy = node->p->y - p->y;
//       auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);
//       auto d3 = d*d*d;

//       p->a_x += (G*node->p->mass*dx)/d3;
//       p->a_y += (G*node->p->mass*dy)/d3;
//     }
//   } else {
//     auto dx = node->com_x/node->n_particles - p->x;
//     auto dy = node->com_y/node->n_particles - p->y;
//     auto d = fmax(sqrt((dx * dx) + (dy * dy)), RLIMIT);

//     if ((node->s_x*2)/d < theta) {
//       // ratio under theta, approximate
//       auto d3 = d*d*d;

//       p->a_x += (G*(node->mass/node->n_particles)*dx)/d3;
//       p->a_y += (G*(node->mass/node->n_particles)*dy)/d3;
//     } else {
//       // ratio over theta, recurse
//       update_force(p, node->nw, theta);
//       update_force(p, node->ne, theta);
//       update_force(p, node->sw, theta);
//       update_force(p, node->se, theta);
//     }
//   }
// }

// void update_forces(const struct quad_tree *root,
//     const struct quad_tree *node, double theta) {
//   if (node == NULL) {
//     return ;
//   } else if (node->p != NULL) {
//     node->p->a_x = 0.0f;
//     node->p->a_y = 0.0f;

//     update_force(node->p, root, theta);
//   } else {
//     update_forces(root, node->nw, theta);
//     update_forces(root, node->ne, theta);
//     update_forces(root, node->sw, theta);
//     update_forces(root, node->se, theta);
//   }
// }

// void update_velocity_position(particle *p, double d_t) {
//     p->x += p->v_x*d_t + 0.5f*p->a_x*POW2(d_t);
//     p->y += p->v_y*d_t + 0.5f*p->a_y*POW2(d_t);

//     if (p->x < MIN_X || p->x > MAX_X
//         || p->y < MIN_Y || p->y > MAX_Y) {
//       p->mass = OUT_OF_BOUNDS_MASS;
//     }

//     p->v_x += p->a_x*d_t;
//     p->v_y += p->a_y*d_t;
// }

void barnes_hut_mpi(const struct options_t *args,
    int n_particles,
    struct particle *particles) {

  MPI_Init(NULL, NULL);

  auto start_time = MPI_Wtime();

  // Get the number of processes
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of the process
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  // How to send a struct in MPI
  // https://stackoverflow.com/questions/18165277/how-to-send-a-variable-of-type-struct-in-mpi-send
  // MPI_Datatype array_of_types[count];
  // // You just have int
  // array_of_types[0] = MPI_INT;

  // // Says how many elements for block
  // int array_of_blocklengths[count];
  // // You have 8 int
  // array_of_blocklengths[0] = {8};

  // [> Says where every block starts in memory, counting from the beginning of the struct. <]
  // MPI_Aint array_of_displaysments[coun];
  // MPI_Aint address1, address2;
  // MPI_Get_address(&_info,&address1);
  // MPI_Get_address(&_info.ne,&address2);
  // array_of_displaysments[0] = address2 - address1;

  // [>Create MPI Datatype and commit<]
  // MPI_Datatype stat_type;
  // MPI_Type_create_struct(count, array_of_blocklengths, array_of_displaysments, array_of_types, &stat_type);
  // MPI_Type_commit(&stat_type);

  for (int i = 0; i < args->steps; i++) {
    DEBUG_PRINT(printf("Step %d\n", i));

    quad_tree *root = build_quad_tree(n_particles, particles);

    if (args->visualization) {
      bool quit = render_visualization(n_particles, particles, root);

      if (quit) {
        return ;
      }
    }

    for (int i = world_rank; i < n_particles; i += world_size) {
      update_force(particles + i, root, args->theta);
    }

    for (int i = world_rank; i < n_particles; i += world_size) {
      update_velocity_position(particles + i, args->delta);
    }

    free_quad_tree(root);
  }

  auto end_time = MPI_Wtime();
  TIMING_PRINT(printf("MPI Overall: %lf\n", end_time - start_time));

  MPI_Finalize();

}

