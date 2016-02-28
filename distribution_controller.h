#ifndef __scable_distribution_controller__
#define __scable_distribution_controller__

#include "rte.h"

#include "pkt_action.h"
#include "lcore_action.h"
#include "lcore.h"

class DistributionController {
  private:
    Rte::Name distName;
    struct rte_distributor *distributor;
    std::vector<struct rte_ring *> rxRings;
    std::vector<int> workers;
    const LcoreActionDelegate<DistributionController> lcoreActionDelegate;

    DistributionController() : lcoreActionDelegate(this) {
    }
  public:
    ~DistributionController() {
      if (distributor) {
        // api call missing rte_distributor_free(distributor);
      }
    }

    static DistributionController *create() {
      auto dc = new DistributionController();
      return dc;
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
      return "DistributionController";
    }

    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this << "=>" << name();
      /*
      distributor = rte_distributor_create(dc->distName("dc:dist:"),
        rte_socket_id(), workers.size());
      if (dc->distributor == NULL) {
        LOG(ERROR) << "Cannot create distributor";
        return 0;
      }
      */
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
      // Transmit to tx
      for (int i = 0; i < rx_size; ++i) {
        rte_pktmbuf_free(pkts_burst[i]);
      }
    }
};

#endif
