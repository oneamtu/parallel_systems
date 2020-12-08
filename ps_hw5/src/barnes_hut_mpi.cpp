#include "barnes_hut_mpi.h"

#include <chrono>
#include <cmath>
#include <csignal>
#include <mpi.h>

#include "io.h"
#include "barnes_hut_sequential.h"
#include "visualization.h"

void barnes_hut_mpi(const struct options_t *args) {
  // Start timer
  auto start = std::chrono::high_resolution_clock::now();

  MPI_Init(NULL, NULL);

  // Get the number of processes
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of the process
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  int n_particles;
  particle *particles;

  if (world_rank == 0) {
    read_file(args, &n_particles, &particles);
  }

  particle dummy_particle;

  // How to send a struct in MPI
  // https://stackoverflow.com/questions/18165277/how-to-send-a-variable-of-type-struct-in-mpi-send
  // https://www.howtobuildsoftware.com/index.php/how-do/OO0/c-mpi-mpi-send-struct-dynamic-memory-allocation
  // Let's make a type for particle
  MPI_Datatype types[] = {MPI_INT, MPI_DOUBLE};
  int block_lengths[] = {1, 7};
  MPI_Aint displacements[] = {
    (int *)(&dummy_particle.index) - (int *)(&dummy_particle),
    (int *)(&dummy_particle.x) - (int *)(&dummy_particle)
  };

  MPI_Datatype particle_mpi_type;
  MPI_Type_create_struct(2, block_lengths, displacements, types, &particle_mpi_type);
  MPI_Type_commit(&particle_mpi_type);

  // sync particles
  MPI_Bcast(&n_particles, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (world_rank != 0) {
    //alloc particles for non-root
    particles = (particle *) malloc(n_particles * sizeof(particle));
  }

  MPI_Bcast(particles, n_particles, particle_mpi_type, 0, MPI_COMM_WORLD);

  // static partitioning
  int partition_size = n_particles / world_size + (n_particles % world_size == 0 ? 0 : 1);
  int *partition_sizes = (int *)malloc(world_size * sizeof(int));
  int *partition_starts = (int *)malloc(world_size * sizeof(int));
  int *partition_ends = (int *)malloc(world_size * sizeof(int));
  particle **particle_buffers = (particle **)malloc(world_size * sizeof(particle *));

  for (int i = 0; i < world_size; i++) {
    partition_starts[i] = i * partition_size;
    partition_ends[i] = std::min((i + 1) * partition_size, n_particles);
    partition_sizes[i] = partition_ends[i] - partition_starts[i];
  }

  double build_tree_timing = 0.0f,
         update_force_timing = 0.0f,
         update_velocity_position_timing = 0.0f,
         free_tree_timing = 0.0f,
         particle_sync_timing = 0.0f;

  for (int i = 0; i < args->steps; i++) {
    DEBUG_PRINT(printf("Step %d\n", i));

    auto iteration_start_time = MPI_Wtime();
    quad_tree *root = build_quad_tree(n_particles, particles);
    build_tree_timing += MPI_Wtime() - iteration_start_time;

    if (args->visualization && world_rank == 0) {
      bool quit = render_visualization(n_particles, particles, root);

      if (quit) {
        return ;
      }
    }

    iteration_start_time = MPI_Wtime();
    for (int i = partition_starts[world_rank]; i < partition_ends[world_rank]; i++) {
      particles[i].a_x = 0.0f;
      particles[i].a_y = 0.0f;

      update_force(particles + i, root, args->theta);
    }
    update_force_timing += MPI_Wtime() - iteration_start_time;

    iteration_start_time = MPI_Wtime();
    for (int i = partition_starts[world_rank]; i < partition_ends[world_rank]; i++) {
      update_velocity_position(particles + i, args->delta);
    }
    update_velocity_position_timing += MPI_Wtime() - iteration_start_time;

    iteration_start_time = MPI_Wtime();
    free_quad_tree(root);
    free_tree_timing += MPI_Wtime() - iteration_start_time;

    iteration_start_time = MPI_Wtime();
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
        particles, partition_sizes, partition_starts, particle_mpi_type, MPI_COMM_WORLD);
    particle_sync_timing += MPI_Wtime() - iteration_start_time;
  }

  if (world_rank == 0) {
    write_file(args, n_particles, particles);
  } else {
    //alloc particles for non-root
    free(particles);
  }

  free(partition_sizes);
  free(partition_starts);
  free(partition_ends);
  free(particle_buffers);

  TIMING_PRINT(printf("P%d build_tree: %lf\n", world_rank,
        (build_tree_timing/args->steps) * MS_PER_S));
  TIMING_PRINT(printf("P%d update_force: %lf\n", world_rank,
        (update_force_timing/args->steps) * MS_PER_S));
  TIMING_PRINT(printf("P%d update_velocity_position: %lf\n",
        world_rank, (update_velocity_position_timing/args->steps) * MS_PER_S));
  TIMING_PRINT(printf("P%d free_tree: %lf\n", world_rank,
        (free_tree_timing/args->steps) * MS_PER_S));
  TIMING_PRINT(printf("P%d particle_sync: %lf\n", world_rank,
        (free_tree_timing/args->steps) * MS_PER_S));

  if (world_rank == 0) {
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    TIMING_PRINT(printf("overall per iteration: %lf\n", diff.count() / 1.0f / args->steps));
    TIMING_PRINT(printf("overall per iteration/particle: %lf\n", diff.count() / 1.0f / n_particles / args->steps));
    TIMING_PRINT(printf("overall: %lf\n", diff.count() / 1.0f));
    std::cout << diff.count() / MS_PER_S << std::endl;
  }

  MPI_Finalize();
}
