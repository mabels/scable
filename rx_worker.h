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
    void main_loop();
  public:
    // static int launch(void *dummy) {
    //   static_cast<RxWorker *>(dummy)->main_loop();
    //   return 0;
    // }
    
    RxWorker(const RxWorkers &rxWorkers, Port &port)
      : port(port), rxWorkers(rxWorkers) {
    }

};

#endif
