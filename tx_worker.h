#ifndef __scable_tx_worker__
#define __scable_tx_worker__

#include "port.h"

class TxWorkers;

class TxWorker {
  private:
    Port &port;
    const TxWorkers &rxWorkers;
    int lcore_id;
    struct rte_mbuf *pkts_burst[Port::MAX_PKT_BURST];
    void main_loop();
  public:
    static int launch(void *dummy) {
      static_cast<TxWorker *>(dummy)->main_loop();
      return 0;
    }

    TxWorker(const TxWorkers &rxWorkers, Port &port)
      : port(port), rxWorkers(rxWorkers) {
    }

};

#endif
