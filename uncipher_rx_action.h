#ifndef __scable_uncipher_rx_action__
#define __scable_uncipher_rx_action__

#include "action_ref.h"
#include "cipher_controller.h"

class UncipherRXAction {
  private:
    Port *port;
    ActionRef<UncipherRXAction> actionRef;
    CipherController &cipherController;
  public:
    UncipherRXAction(CipherController &cipherController)
    : actionRef(this), cipherController(cipherController) {
    }

    void bindPort(Port *port) {
      this->port = port;
    }

    void action(Lcore &lcore) {
      /*
       * Read packet from RX queues
       */
      struct rte_mbuf *pkts_burst[Port::MAX_PKT_BURST];
      const uint16_t nb_rx = rte_eth_rx_burst(port->getId(), 0, pkts_burst, Port::MAX_PKT_BURST);
      port->getPortStatistics().rx += nb_rx;
      // if (nb_rx) {
      //   LOG(INFO) << "received:" << name() << ":" << port->getId() << ":" << nb_rx;
      // }
      rte_distributor_process(cipherController.getDistributor(), pkts_burst, nb_rx);
    }

    const Action& getAction() const {
      return actionRef.get();
    }

    const char *name() const {
      return "UncipherRXAction";
    }

};

#endif
