#ifndef __scable_rte_controller__
#define __scable_rte_controller__

#include <memory>
#include <vector>
#include "rte.h"

#include "lcores.h"
#include "ports.h"

class RteController {
private:
  Rte rte;
  struct rte_distributor *cryptWorkers;

  Ports ports;
  Lcores lcores;

public:
  bool start(int argc, char **argv);
};

#endif
