#ifndef __scable_cipher_rx_action__
#define __scable_cipher_rx_action__

#include "action_ref.h"
#include "cipher_controller.h"

class CipherRXAction {
  private:
    Port *port;
    ActionRef<CipherRXAction> actionRef;
    CipherController &cipherController;
  public:
    CipherRXAction(CipherController &cipherController)
      : actionRef(this), cipherController(cipherController) {
    }

    void bindPort(Port *port) {
      this->port = port;
    }

    void prepare() {
      //distributor = rxWorkers.getCryptoWorkers().getDistributor();
    }
    void action(Lcore &lcore) {
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
        rte_distributor_process(cipherController.getDistributor(), pkts_burst, nb_rx);
    }

    const Action& getAction() const {
      return actionRef.get();
    }
    const char *name() const {
      return "CipherRXAction";
    }
};

#endif
