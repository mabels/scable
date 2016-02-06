#ifndef __scable_tx_workers__
#define __scable_tx_workers__

#include "rte_controller.h"
#include "tx_worker.h"

class RteController;
class CryptoWorkers;

class Port;

class TxWorkers {
private:
  std::vector<std::unique_ptr<TxWorker>> workers;
  typedef std::map<uint16_t, Port> Ports;
  Ports ports;
  RteController &rtc;
  CryptoWorkers &cws;
public:
  TxWorkers(RteController &rtc, CryptoWorkers &cws)
    : rtc(rtc), cws(cws) {
  }

  TxWorkers *addPort(uint16_t portid);

  RteController &getRtc() const {
    return rtc;
  }
  CryptoWorkers &getCryptoWorkers() const {
    return cws;
  }


};

#endif
