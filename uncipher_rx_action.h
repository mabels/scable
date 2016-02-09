#ifndef __scable_uncipher_rx_action__
#define __scable_uncipher_rx_action__

#include "lcore_action.h"
#include "pkt_action.h"
#include "cipher_controller.h"

class UncipherRXAction {
  private:
    Port *port;
    const LcoreActionDelegate<UncipherRXAction> lcoreActionDelegate;
    const PktActionDelegate<UncipherRXAction> uncipherActionDelegate;
    CipherController &cipherController;
  public:
    UncipherRXAction(CipherController &cipherController)
    : lcoreActionDelegate(this),
      uncipherActionDelegate(this),
      cipherController(cipherController) {
    }

    Port *bindPort(Port *port) {
      this->port = port;
      return port;
    }

    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this << "=>" << name();
    }

    void lcoreAction(Lcore &lcore) {
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

    void pktPrepare() {
      LOG(INFO) << "Starting PktAction for Port:" << port->getId() << ":" << this << "=>" << name();
    }

    bool pktAction(struct rte_mbuf *buf) {
      if (buf->port == port->getId()) {
        struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
        LOG(INFO) << "on[" << name() << port->getId() << "] " << eth->s_addr << ">>" << eth->d_addr;
        return true;
      }
      return false;
    }

    const LcoreAction& getAction() const {
      return lcoreActionDelegate.get();
    }

    const PktAction& getCipherAction() const {
      return uncipherActionDelegate.get();
    }

    Port *getPort() const {
      return port;
    }

    const char *name() const {
      return "UncipherRXAction";
    }

};

#endif
