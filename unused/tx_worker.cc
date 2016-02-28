
#include <iomanip>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "rte.h"
#include "tx_worker.h"
#include "tx_workers.h"
#include "crypto_workers.h"

#ifdef XXXXX
void tx_handler() {
  /*
   * TX burst queue drain
   */
  const uint64_t diff_tsc = cur_tsc - prev_tsc;
  if (unlikely(diff_tsc > drain_tsc)) {

    for (portid = 0; portid < RTE_MAX_ETHPORTS; ++portid) {
      if (qconf->tx_mbufs[portid].len == 0)
        continue;
      l2fwd_send_burst(&lcore_queue_conf[lcore_id], qconf->tx_mbufs[portid].len,
                       (uint8_t)portid);
      qconf->tx_mbufs[portid].len = 0;
    }

    /* if timer is enabled */
    if (timer_period > 0) {

      /* advance the timer */
      timer_tsc += diff_tsc;

      /* if timer has reached its timeout */
      if (unlikely(timer_tsc >= (uint64_t)timer_period)) {

        /* do this only on master core */
        if (lcore_id == rte_get_master_lcore()) {
          print_stats();
          /* reset the timer */
          timer_tsc = 0;
        }
      }
    }

    prev_tsc = cur_tsc;
  }
}
#endif

void TxWorker::main_loop() {
  lcore_id = rte_lcore_id();
  const uint64_t drain_tsc =
      (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * port.BURST_TX_DRAIN_US;
  LOG(INFO) << "entering TxWorker::main_loop on lcore " << lcore_id;

  // struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
  // struct rte_mbuf *m;
  // uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
  // unsigned i, j, portid, nb_rx;

  // for (unsigned i = 0; i < qconf.n_rx_port; i++) {
  //   portid = qconf.rx_port_list[i];
  //   LOG(INFO) <<  " -- lcoreid=" << lcore_id << " portid=" << portid;
  // }
  uint64_t prev_tsc = 0;
  uint64_t timer_tsc = 0;
  struct rte_distributor *distributor = rxWorkers.getCryptoWorkers().getDistributor();
  while (1) {
    const uint64_t cur_tsc = rte_rdtsc();
    /*
     * Read packet from RX queues
     */
    const uint16_t nb_rx =
        rte_eth_rx_burst(port.getId(), 0, pkts_burst, port.MAX_PKT_BURST);
    port.getPortStatistics().rx += nb_rx;
//    if (nb_rx) {
//      LOG(INFO) << "received:" << nb_rx;
//    }
    rte_distributor_process(distributor, pkts_burst, nb_rx);

    // sleep missing if idle
  }
}
