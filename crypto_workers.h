#ifndef __scable_crypto_workers__
#define __scable_crypto_workers__

#include "rte_controller.h"

class RteController;

template<
class CryptoWorkers {
private:
  std::vector<std::unique_ptr<CryptoWorker>> workers;
  static int launch(void *dummy) {
    static_cast<CryptoWorker *>(dummy)->main_loop();
    return 0;
  }
  CryptoWorker(RteController &rtc) : rtc(rtc) {
  }
public:

  static CryptoWorkers *start(RteController &rtc, int num) {
    auto cw = new CryptoWorkers(rtc);

    return cw;
  }

  RxWorkers *addPort(uint16_t portid);

  RteController &getRtc() const {
    return rtc;
  }
};

#endif
