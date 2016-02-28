#ifndef __scable_decipher_worker__
#define __scable_decipher_worker__

#include "launch_params.h"

class CryptoWorkers;

class DecipherWorker {
  private:
    CryptoWorkers &cryptoWorkers;
    int lcore_id;

    static int launch(void *dummy) {
      static_cast<DecipherWorker *>(dummy)->main_loop();
      return 0;
    }
  public:
    static LaunchParams factory(CryptoWorkers &cws) {
      LaunchParams fr;
      fr.context = new DecipherWorker(cws);
      fr.launch = launch;
      return fr;
    }

    DecipherWorker(CryptoWorkers &cryptoWorkers)
      : cryptoWorkers(cryptoWorkers) {
    }
    void main_loop();

};

#endif
