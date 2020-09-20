#pragma once

#include <stdlib.h>
#include <pthread.h>
#include "spin_barrier.h"
#include <iostream>

void* compute_prefix_sum(void* a);
void *compute_prefix_parallel_tree_sum(void *a);
void *compute_prefix_parallel_block_parallel_sum(void *a);
void *compute_prefix_parallel_block_sequential_sum(void *a);
