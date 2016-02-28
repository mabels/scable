#ifndef __scable_rx_action__
#define __scable_rx_action__

#include "lcore_action.h"
#include "pkt_action.h"
#include "cipher_controller.h"

class Cipher2UncipherWorker;
class Uncipher2CipherWorker;

class RXAction {
  private:
    Port *port;
    const LcoreActionDelegate<RXAction> lcoreActionDelegate;
    RXAction()
      : lcoreActionDelegate(this) {
    }
  public:
    ~RXAction() {
    }

    static RXAction *create() {
      auto ret = new RXAction();
      // ret->distributorRing = rte_ring_create(ret->distributorRingName("RXAction:"),
      //                                       CipherController::RTE_RING_SZ, rte_socket_id(), RING_F_SC_DEQ);
      // if (ret->distributorRing == NULL) {
      //   LOG(ERROR) << "Cannot create output ring";
      //   delete ret;
      //   return 0;
      // }
      return ret;
    }

    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this
        << "=>" << name() << " for port " << port->getId();
    }
    void lcoreAction(Lcore &lcore) {
        // const uint64_t drain_tsc =
        //     (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * port.BURST_TX_DRAIN_US;
        // LOG(INFO) << "entering RxWorker::main_loop on lcore " << lcore_id;

        // struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
        // struct rte_mbuf *m;
        // uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
        // unsigned i, j, portid, nb_rx;

        // for (unsigned i = 0; i < qconf.n_rx_port; i++) {
        //   portid = qconf.rx_port_list[i];
        //   LOG(INFO) <<  " -- lcoreid=" << lcore_id << " portid=" << portid;
        // }
        // uint64_t prev_tsc = 0;
        // uint64_t timer_tsc = 0;
        //const uint64_t cur_tsc = rte_rdtsc();
        /*
         * Read packet from RX queues
         */
        struct rte_mbuf *pkts_burst[Port::MAX_PKT_BURST];
        const uint16_t nb_rx = rte_eth_rx_burst(port->getId(), 0, pkts_burst, Port::MAX_PKT_BURST);
        port->getPortStatistics().rx += nb_rx;
        // if (nb_rx) {
        //   LOG(INFO) << "received:" << port->getId() << ":" << nb_rx;
        // }
        // unsigned ret = rte_ring_sp_enqueue_burst(distributorRing, (void **)pkts_burst, nb_rx);
        // if (ret != nb_rx) {
        //   LOG(ERROR) << "lcoreAction:rte_ring_sp_enqueue_burst missed some packet " << nb_rx << ":" << ret;
        // }
        // if (nb_rx > 0) {
        //   unsigned rc = rte_ring_count(distributorRing);
        //   // LOG(INFO) << "cipherRXAction:" << distributorRing << ":" << port->getPortStatistics().rx << ":" << rc << ":" << nb_rx << ":" << ret;
        // }
    }

    const LcoreAction& getAction() const {
      return lcoreActionDelegate.get();
    }

    Port * getPort() const {
      return port;
    }

    const char *name() const {
      return "RXAction";
    }

    RXAction &bindPort(Port &p) {
      port = &p;
      return *this;
    }

    RXAction &bindWorker(Cipher2UncipherWorker &c) {
      //cipher2UncipherWorker = &c;
      return *this;
    }
    RXAction &bindWorker(Uncipher2CipherWorker &c) {
      //uncipher2CipherWorker = &c;
      return *this;
    }

    std::string toString() {
      return port->toString();
    }

};

#endif
