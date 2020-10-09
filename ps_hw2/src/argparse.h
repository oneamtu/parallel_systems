#pragma once

#include <getopt.h>
#include <stdlib.h>
#include <iostream>

struct options_t {
    int n_clusters;
    int dimensions;
    char *in_file;
    int max_iterations;
    double threshold;
    bool print_centroids;
    int seed;
    int algorithm;
};

void get_opts(int argc, char **argv, struct options_t *opts);
