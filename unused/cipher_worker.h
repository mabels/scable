#ifndef __scable_cipher_worker__
#define __scable_cipher_worker__

#include "launch_params.h"

class CryptoWorkers;

class CipherWorker {
  private:
    CryptoWorkers &cryptoWorkers;
    int lcore_id;

    static int launch(void *dummy) {
      static_cast<CipherWorker *>(dummy)->main_loop();
      return 0;
    }
  public:
    static LaunchParams factory(CryptoWorkers &cws) {
      LaunchParams fr;
      fr.context = new CipherWorker(cws);
      fr.launch = launch;
      return fr;
    }

    CipherWorker(CryptoWorkers &cryptoWorkers)
      : cryptoWorkers(cryptoWorkers) {
    }
    void main_loop();

};

#endif
