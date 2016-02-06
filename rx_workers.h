#ifndef __scable_rx_workers__
#define __scable_rx_workers__

#include "rte_controller.h"
#include "rx_worker.h"

class RteController;

class Port;

class RxWorkers {
private:
  std::vector<std::unique_ptr<RxWorker>> workers;
  typedef std::map<uint16_t, Port> Ports;
  Ports ports;
  RteController &rtc;
  static int launch(void *dummy) {
    static_cast<RxWorker *>(dummy)->main_loop();
    return 0;
  }
public:
  RxWorkers(RteController &rtc) : rtc(rtc) {
  }

  RxWorkers *addPort(uint16_t portid);

  RteController &getRtc() const {
    return rtc;
  }
};

#endif
