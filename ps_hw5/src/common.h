#pragma once

#include <iostream>
#include <cstring>
#include <stdlib.h>

// constants
const static double G = 0.0001;
const static double RLIMIT = 0.03;
const static double DT = 0.05;

const static double MIN_X = 0.0;
const static double MAX_X = 4.0;

const static double MIN_Y = 0.0;
const static double MAX_Y = 4.0;

//useful macros
#ifdef TIMING
#define TIMING_TEST 1
#else
#define TIMING_TEST 0
#endif

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define TIMING_OUT(x) do { if (DEBUG_TEST) { std::cerr << "TIMING: " << (x) << std::endl;} } while (0)
#define TIMING_PRINT(x) do { if (TIMING_TEST) { printf("TIMING: "); x;} } while (0)
#define DEBUG_OUT(x) do { if (DEBUG_TEST) { std::cerr << "DEBUG: " << (x) << std::endl;} } while (0)
#define DEBUG_PRINT(x) do { if (DEBUG_TEST) { printf("DEBUG: "); x;} } while (0)

#define POW2(x) (x)*(x)

#define HANDLE(x) \
  do { \
    int res = x; \
    if (res) {\
      std::cerr << "[" << __FILE__ << "][" << __FUNCTION__ << "][Line " << __LINE__ << "] "\
        << #x << " " << strerror(res) << std::endl;\
      exit(1);\
    }\
  } while (0) \

