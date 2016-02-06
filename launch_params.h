#ifndef __scable_launch_params__
#define __scable_launch_params__

#include "rte.h"

typedef struct S_LaunchParams {
  lcore_function_t *launch;
  void *context;
} LaunchParams;

#endif
