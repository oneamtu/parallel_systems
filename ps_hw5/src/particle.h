#pragma once

static const double OUT_OF_BOUNDS_MASS = -1.0f;

struct particle {
    int index;
    double x;
    double y;
    double mass;
    double v_x;
    double v_y;

    double a_x;
    double a_y;
};

struct quad_tree {
  particle *p;

  double mass;
  double com_x;
  double com_y;

  double partition_x;
  double partition_y;
  double s_x;
  double s_y;

  int n_particles;

  quad_tree *nw;
  quad_tree *ne;
  quad_tree *sw;
  quad_tree *se;
};

