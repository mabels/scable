#ifndef __scable_rx_worker__
#define __scable_rx_worker__

#include "port.h"

class RxWorkers;

class RxWorker {
  private:
    Port &port;
    const RxWorkers &rxWorkers;
    int lcore_id;
    struct rte_mbuf *pkts_burst[Port::MAX_PKT_BURST];
  public:
    RxWorker(const RxWorkers &rxWorkers, Port &port)
      : port(port), rxWorkers(rxWorkers) {
    }
    void main_loop();

};

#endif
