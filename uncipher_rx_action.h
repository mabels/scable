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
    struct rte_ring *distributorRing;
    Rte::Name distributorRingName;
    UncipherRXAction(CipherController &cipherController)
      : lcoreActionDelegate(this),
        uncipherActionDelegate(this),
        cipherController(cipherController) {
    }
  public:
    ~UncipherRXAction() {
      if (distributorRing) {
        rte_ring_free(distributorRing);
      }
    }
    static UncipherRXAction *create(CipherController &cipherController) {
      auto ret = new UncipherRXAction(cipherController);
      ret->distributorRing = rte_ring_create(ret->distributorRingName("UncipherRXAction:"),
                                            CipherController::RTE_RING_SZ, rte_socket_id(), RING_F_SC_DEQ);
      if (ret->distributorRing == NULL) {
        LOG(ERROR) << "Cannot create output ring";
        delete ret;
        return 0;
      }
      return ret;
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
      // unsigned ret = 0;
      unsigned ret = rte_ring_sp_enqueue_burst(distributorRing, (void **)pkts_burst, nb_rx);
      if (ret != nb_rx) {
        LOG(ERROR) << "lcoreAction:rte_ring_sp_enqueue_burst missed some packet " << nb_rx << ":" << ret;
      }
      if (nb_rx > 0) {
        unsigned rc = rte_ring_count(distributorRing);
        // LOG(INFO) << "UncipherRXAction:" << distributorRing << ":" << port->getPortStatistics().rx << ":" << rc << ":" << nb_rx << ":" << ret;
      }
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

    struct rte_ring *getDistributorRing() const {
      return distributorRing;
    }


};

#endif
