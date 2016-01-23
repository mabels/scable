#ifndef __scable_rte_controller__
#define __scable_rte_controller__

#include <memory>
#include <vector>
#include "rte.h"
#include "port_controller.h"

class PortController;

class RteController {
private:
  const static uint16_t NB_MBUF = 8192;
  Rte rte;
  struct rte_mempool *pktmbuf_pool;
  std::vector<std::unique_ptr<PortController>> ports;

public:
  struct rte_mempool *getPktMbufPool() const {
    return pktmbuf_pool;
  }
  bool start(int argc, char **argv);
};

#endif
