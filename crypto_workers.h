#ifndef __scable_crypto_workers__
#define __scable_crypto_workers__

#include "rte.h"
#include "rte_controller.h"
#include "launch_params.h"
#include "cipher_worker.h"
#include "decipher_worker.h"

class RteController;

class CryptoWorkers {
private:
  static const int RTE_RING_SZ = 1024;
  std::vector<LaunchParams> workers;
  struct rte_distributor *distributor;
  struct rte_ring *outputRing;
  RteController &rtc;
  Rte::Name distName;
  Rte::Name outpName;
  CryptoWorkers(RteController &rtc) : rtc(rtc) {
  }
public:
  static CryptoWorkers *create(RteController &rtc,
    LaunchParams (*factory)(CryptoWorkers &cws), int numWorkers) {
    auto cws = new CryptoWorkers(rtc);
    std::ostringstream distName;
    cws->distributor = rte_distributor_create(cws->distName("cws:dist:"), rte_socket_id(), numWorkers);
    if (cws->distributor == NULL) {
      LOG(ERROR) << "Cannot create distributor";
      return 0;
    }

    cws->outputRing = rte_ring_create(cws->outpName("cws:outp:"), RTE_RING_SZ,
                rte_socket_id(), RING_F_SC_DEQ);
    if (cws->outputRing == NULL) {
      LOG(ERROR) << "Cannot create output ring";
      return 0;
    }

    cws->workers.resize(numWorkers);
    // for(int i = 0; i < numWorkers; ++i) {
    //   auto lp = (*factory)(*cws);
    //   rtc.launch(lp.launch, lp.context);
    //   cws->workers[i] = lp;
    // }
    return cws;
  }

  RteController &getRtc() const {
    return rtc;
  }

  struct rte_distributor *getDistributor() const {
    return distributor;
  }
  struct rte_ring *getOutputRing() const {
    return outputRing;
  }
};

#endif
