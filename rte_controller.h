#ifndef __scable_rte_controller__
#define __scable_rte_controller__

#include <memory>
#include <vector>
#include "rte.h"


class RteController {
private:
  Rte rte;
public:
  bool start(int argc, char **argv);
};

#endif
