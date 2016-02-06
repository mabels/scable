#ifndef __scable_rte_controller__
#define __scable_rte_controller__

#include <memory>
#include <vector>
#include "rte.h"


class RteController {
private:
  const static uint16_t NB_MBUF = 8192;
  Rte rte;
  struct rte_mempool *pktmbuf_pool;
  struct rte_distributor *cryptWorkers;

  std::vector<bool> usedLcores;

public:
  struct rte_mempool *getPktMbufPool() const {
    return pktmbuf_pool;
  }
  bool start(int argc, char **argv);
  bool launch(lcore_function_t *f, void *arg);
};

#endif
