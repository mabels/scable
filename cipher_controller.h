#ifndef __scable_cipher_controller__
#define __scable_cipher_controller__

#include "rte.h"

#include "pkt_action.h"
#include "lcore_action.h"

class CipherController {
  public:
    static const int RTE_RING_SZ = 1024;
  private:
    Rte::Name distName;
    struct rte_distributor *distributor;

    std::vector<struct rte_ring *> rxRings;

    LcoreActionDelegate<CipherController> lcoreActionDelegate;
    std::vector<PktAction> cipherActions;
    struct rte_mbuf *buf = 0;

    CipherController() : lcoreActionDelegate(this) {
    }

  public:
    static CipherController *create(int numWorkers) {
      auto cctl = new CipherController();
      cctl->distributor = rte_distributor_create(cctl->distName("cctl:dist:"), rte_socket_id(), numWorkers);
      if (cctl->distributor == NULL) {
        LOG(ERROR) << "Cannot create distributor";
        return 0;
      }
      return cctl;
    }

    void addRxRing(struct rte_ring *ring) {
      rxRings.push_back(ring);
    }

    struct rte_distributor *getDistributor() const {
      return distributor;
    }
    const LcoreAction& getAction() const {
      return lcoreActionDelegate.get();
    }
    const char *name() const {
      return "CipherController";
    }

    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this << "=>" << name();
      for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
        (*(it->prepare))(it->context);
      }
    }
    void lcoreAction(Lcore &lcore) {
      const int size = Port::MAX_PKT_BURST * rxRings.size();
      struct rte_mbuf *pkts_burst[size];
      for (auto it : rxRings) {
        unsigned nb_rx = rte_ring_sc_dequeue_burst(it, (void **)pkts_burst, Port::MAX_PKT_BURST);
        if (nb_rx > 0) {
          unsigned rc = rte_ring_count(it);
          // for (int i = 0; i < nb_rx; ++i) {
          //   rte_pktmbuf_free(pkts_burst[i]);
          // }
          rte_distributor_process(distributor, pkts_burst, nb_rx);
        }
      }
      unsigned rx_size = rte_distributor_returned_pkts(distributor, pkts_burst, size);
      if (rx_size) {
        LOG(INFO) << "rte_distributor_returned_pkts:" << rx_size;
      }
      for (int i = 0; i < rx_size; ++i) {
        rte_pktmbuf_free(pkts_burst[i]);
      }

      // buf = rte_distributor_get_pkt(distributor, lcore.getId(), buf);
      // rte_prefetch0(rte_pktmbuf_mtod(m, void *));
      //struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
      // for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
      //   if ((*(it->action))(it->context, buf)) {
      //       break;
      //   }
      // }
    }

    bool addCipherAction(const PktAction &pktAction) {
      for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
          if ((*(pktAction.getport))(pktAction.context)->getId() == (*(it->getport))(it->context)->getId()) {
            LOG(ERROR) << "redefined port for cipherAction";
            return false;
          }
      }
      cipherActions.push_back(pktAction);
      return true;
    }
};

#endif
