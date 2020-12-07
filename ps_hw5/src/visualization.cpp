#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/freeglut.h>

#include "visualization.h"

#define WIN_X(x) (2*(x)/MAX_X - 1)
#define WIN_Y(y) (2*(y)/MAX_Y - 1)

const static int WIN_WIDTH=1000, WIN_HEIGHT=1000;

static bool quit = false;
static bool running = false;
static bool next_frame = false;

static GLFWwindow* window;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
    DEBUG_OUT("GLFW_KEY_Q -> quit");

    quit = true;
  } else if (key == GLFW_KEY_J && action == GLFW_PRESS) {
    DEBUG_OUT("GLFW_KEY_J -> next_frame");

    next_frame = true;
  } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
    DEBUG_OUT("GLFW_KEY_L -> running");

    running = !running;
  }
}

int init_visualization(int *argc, char **argv) {
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n" );
    return -1;
  }
  // Open a window and create its OpenGL context
  window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Simulation", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to open GLFW window.\n" );
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  // // Initialize GLEW
  // if (glewInit() != GLEW_OK) {
  //     fprintf(stderr, "Failed to initialize GLEW\n");
  //     return -1;
  // }
  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  glfwSetKeyCallback(window, key_callback);

  glutInit(argc, argv);

  return 0;
}

void terminate_visualization() {
  glfwTerminate();
}

void drawOctreeBounds2D(const quad_tree *node) {
  int i;

  if(node == NULL || node->p != NULL) {
    return;
  }

  glBegin(GL_LINES);
  // set the color of lines to be white
  glColor4f(0.8f, 0.8f, 0.8f, 0.7f);

  glVertex2f(WIN_X(node->partition_x), WIN_Y(node->partition_y - node->s_y));
  glVertex2f(WIN_X(node->partition_x), WIN_Y(node->partition_y + node->s_y));

  glVertex2f(WIN_X(node->partition_x - node->s_x), WIN_Y(node->partition_y));
  glVertex2f(WIN_X(node->partition_x + node->s_x), WIN_Y(node->partition_y));

  glEnd();

  // node text info
  glColor4f(0.8f, 0.8f, 0.2f, 1.0f);
  glRasterPos2f(WIN_X(node->partition_x), WIN_Y(node->partition_y) + 0.01f);
  auto n_particles_s = std::to_string(node->n_particles);
  auto mass_s = std::to_string(node->mass/node->n_particles);
  auto com_x_s = std::to_string(node->com_x/node->n_particles);
  auto com_y_s = std::to_string(node->com_y/node->n_particles);
  auto text_s = n_particles_s + "|" + mass_s + "|" + com_x_s + "|" + com_y_s;

  auto text_c = reinterpret_cast<const unsigned char *>(text_s.c_str());
  glutBitmapString(GLUT_BITMAP_HELVETICA_12, text_c);

  drawOctreeBounds2D(node->nw);
  drawOctreeBounds2D(node->ne);
  drawOctreeBounds2D(node->sw);
  drawOctreeBounds2D(node->se);
}

void drawParticle2D(const struct particle *particle) {
  float angle = 0.0f;
  float radius = 0.005f * particle->mass;

  //draw particle
  glBegin(GL_TRIANGLE_FAN);

  glColor3f(0.1f, 0.3f, 0.6f);

  auto win_x = WIN_X(particle->x);
  auto win_y = WIN_Y(particle->y);

  glVertex2f(win_x, win_y);

  for(int i = 0; i < 20; i++) {
    angle=(float) (i)/19*2*3.141592;

    glVertex2f(
        win_x+radius*cos(angle),
        win_y+radius*sin(angle));
  }

  glEnd();

  //draw velocity
  glBegin(GL_LINES);

  // set the color of lines to be red
  glColor3f(1.0f, 0.1f, 0.1f);

  glVertex2f(WIN_X(particle->x), WIN_Y(particle->y));
  glVertex2f(WIN_X(particle->x + particle->v_x), WIN_Y(particle->y + particle->v_y));

  glEnd();

  // particle text info
  glColor4f(0.8f, 0.8f, 0.2f, 1.0f);
  glRasterPos2f(WIN_X(particle->x), WIN_Y(particle->y) + 0.01f);
  auto mass_s = std::to_string(particle->mass);
  auto x_s = std::to_string(particle->x);
  auto y_s = std::to_string(particle->y);
  auto text_s = mass_s + "|" + x_s + "|" + y_s;

  auto text_c = reinterpret_cast<const unsigned char *>(text_s.c_str());
  glutBitmapString(GLUT_BITMAP_HELVETICA_12, text_c);
}

bool render_visualization(int n_particles,
    const struct particle *particles,
    const struct quad_tree *root) {

  glClear(GL_COLOR_BUFFER_BIT);

  drawOctreeBounds2D(root);

  for(int i = 0; i < n_particles; i++) {
    drawParticle2D(&particles[i]);
  }

  // Swap buffers
  glfwSwapBuffers(window);
  glfwPollEvents();

  while(!next_frame && !running && !quit && !glfwWindowShouldClose(window)) {
    usleep(10);
    glfwPollEvents();
  }
  next_frame = false;

  return quit;
}
