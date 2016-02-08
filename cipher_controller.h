#ifndef __scable_cipher_controller__
#define __scable_cipher_controller__

#include "rte.h"

class CipherController {
  private:
    static const int RTE_RING_SZ = 1024;
    Rte::Name distName;
    struct rte_distributor *distributor;
    Rte::Name oRingName;
    struct rte_ring *outputRing;

    ActionRef<CipherController> actionRef;

    CipherController() : actionRef(this) {
    }

  public:
    static CipherController *create(int numWorkers) {
      auto cctl = new CipherController();
      cctl->distributor = rte_distributor_create(cctl->distName("cctl:dist:"), rte_socket_id(), numWorkers);
      if (cctl->distributor == NULL) {
        LOG(ERROR) << "Cannot create distributor";
        return 0;
      }
      cctl->outputRing = rte_ring_create(cctl->oRingName("cctl:oRing:"), RTE_RING_SZ,
                  rte_socket_id(), RING_F_SC_DEQ);
      if (cctl->outputRing == NULL) {
        LOG(ERROR) << "Cannot create output ring";
        return 0;
      }
      return cctl;
    }
    struct rte_distributor *getDistributor() const {
      return distributor;
    }
    struct rte_ring *getOutputRing() const {
      return outputRing;
    }
    const Action& getAction() const {
      return actionRef.get();
    }
    const char *name() const {
      return "CipherController";
    }
    void action(Lcore &lcore) {
      struct rte_mbuf *buf;
      buf = rte_distributor_get_pkt(distributor, lcore.getId(), buf);
      // rte_prefetch0(rte_pktmbuf_mtod(m, void *));
      struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
      LOG(INFO) << "on[" << lcore.getId() << ":" << ((int)buf->port) << "] " << eth->s_addr << ">>" << eth->d_addr;
    }

};

#endif
