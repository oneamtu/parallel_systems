#include "operators.h"

int __attribute__ ((noinline)) op(int a, int b, int n_loop) {
    volatile int acc = 0;
    for (int i = 0; i < n_loop; i++) {
        acc = i % 2 == 0 ? (acc + a)/(b+1) : (acc + b)/(a+1);
    }

    return acc;
}


int add(int a, int b, int __) {
    return a+b;
}