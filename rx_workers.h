#ifndef __scable_rx_workers__
#define __scable_rx_workers__

#include "rte_controller.h"
#include "rx_worker.h"

class RteController;
class CryptoWorkers;

class Port;

class RxWorkers {
private:
  std::vector<std::unique_ptr<RxWorker>> workers;
  typedef std::map<uint16_t, Port> Ports;
  Ports ports;
  RteController &rtc;
  CryptoWorkers &cws;
public:
  RxWorkers(RteController &rtc, CryptoWorkers &cws)
    : rtc(rtc), cws(cws) {
  }

  //RxWorkers *addPort(uint16_t portid);

  RteController &getRtc() const {
    return rtc;
  }
  CryptoWorkers &getCryptoWorkers() const {
    return cws;
  }


};

#endif
