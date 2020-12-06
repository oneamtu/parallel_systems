#pragma once

struct particle {
    int index;
    double x;
    double y;
    double mass;
    double v_x;
    double v_y;
};

struct quad_tree {
  particle *p;

  double mass;
  double com_x;
  double com_y;

  double partition_x;
  double partition_y;

  int n_particles;

  quad_tree *nw;
  quad_tree *ne;
  quad_tree *sw;
  quad_tree *se;
};

