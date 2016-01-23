#ifndef __scable_port_controller__
#define __scable_port_controller__

#include "rte_controller.h"

class RteController;

class PortController {
private:
  const static uint8_t BURST_TX_DRAIN_US = 100; /* TX drain every ~100us */
  const static uint8_t MAX_PKT_BURST = 32;
  const static uint8_t MAX_RX_QUEUE_PER_LCORE = 16;
  const static uint8_t MAX_TX_QUEUE_PER_PORT = 16;
  const static uint16_t RTE_TEST_RX_DESC_DEFAULT = 128;
  const static uint16_t RTE_TEST_TX_DESC_DEFAULT = 512;
  const static uint16_t CHECK_INTERVAL = 100; /* 100ms */
  const static uint16_t MAX_CHECK_TIME = 90;  /* 9s (90 * 100ms) in total */
  uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
  uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;
  const RteController &rtc;
  struct rte_eth_dev_info dev_info;
  const uint16_t portid;
  const uint8_t rx_queue_per_lcore;
  struct mbuf_table {
    unsigned len;
    struct rte_mbuf *m_table[MAX_PKT_BURST];
  };
  struct lcore_queue_conf {
    unsigned n_rx_port;
    uint8_t rx_lcore_id;
    unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
    struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
  };
  struct lcore_queue_conf qconf;
  struct rte_eth_conf port_conf;
  struct ether_addr ether_addr;
  struct PortStatistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
  } portStatistics = {0, 0, 0};
  struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
  unsigned *lcore_id = 0;
  unsigned lcore_id_value;
public:
  PortController(RteController &rtc, uint16_t portid,
                 uint8_t rx_queue_per_lcore = 1)
      : rtc(rtc), portid(portid), rx_queue_per_lcore(rx_queue_per_lcore) {
    LOG(INFO) << "Created PortController for " << portid;
    rte_eth_dev_info_get(portid, &dev_info);
    memset(&qconf, 0, sizeof(qconf));
    port_conf.rxmode.split_hdr_size = 0;
    port_conf.rxmode.header_split = 0;   /**< Header Split disabled */
    port_conf.rxmode.hw_ip_checksum = 0; /**< IP checksum offload disabled */
    port_conf.rxmode.hw_vlan_filter = 0; /**< VLAN filtering disabled */
    port_conf.rxmode.jumbo_frame = 0;    /**< Jumbo Frame Support disabled */
    port_conf.rxmode.hw_strip_crc = 0;   /**< CRC stripped by hardware */
    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
  }

  bool initialzePort();

  unsigned *getLcoreId() const {
    return lcore_id;
  }

  bool assignLcore2Queue() {
    /* get the lcore_id for this port */
    uint16_t rx_lcore_id;
    for (rx_lcore_id = 0; rte_lcore_is_enabled(rx_lcore_id) == 0 ||
                                  qconf.n_rx_port == rx_queue_per_lcore;
         ++rx_lcore_id) {
      if (rx_lcore_id >= RTE_MAX_LCORE) {
        LOG(ERROR) << "Not enough cores";
        return false;
      }
    }
    // if (qconf != &lcore_queue_conf[rx_lcore_id])
    //   /* Assigned a new logical core in the loop above. */
    //   qconf = &lcore_queue_conf[rx_lcore_id];
    qconf.rx_port_list[qconf.n_rx_port] = portid;
    qconf.n_rx_port++;
    LOG(INFO) << "Lcore " << rx_lcore_id << ": RX port " << portid;
    return true;
  }
  bool checkLinkStatus() {
    LOG(INFO) << "Checking link status";
    struct rte_eth_link link;
    for (uint16_t count = 0; count <= MAX_CHECK_TIME; count++) {
      memset(&link, 0, sizeof(link));
      rte_eth_link_get_nowait(portid, &link);
      /* print link status if flag set */
      if (link.link_status) {
        LOG(INFO) << "Port " << portid << " Link Up - speed " << link.link_speed
                  << "Mbps - " << ((link.link_duplex == ETH_LINK_FULL_DUPLEX)
                                       ? ("full-duplex")
                                       : ("half-duplex"));
        break;
      }
      rte_delay_ms(CHECK_INTERVAL);
    }
    if (!link.link_status) {
      LOG(INFO) << "Port " << portid << " Link Down\n";
    }
    return link.link_status;
  }
  static int launch(void *dummy) {
    static_cast<PortController *>(dummy)->main_loop();
    return 0;
  }
  void tx_handler() {
#ifdef XXXXX
    const uint64_t cur_tsc = rte_rdtsc();
    /*
     * TX burst queue drain
     */
    const uint64_t diff_tsc = cur_tsc - prev_tsc;
    if (unlikely(diff_tsc > drain_tsc)) {

      for (portid = 0; portid < RTE_MAX_ETHPORTS; ++portid) {
        if (qconf->tx_mbufs[portid].len == 0)
          continue;
        l2fwd_send_burst(&lcore_queue_conf[lcore_id],
                         qconf->tx_mbufs[portid].len, (uint8_t)portid);
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
#endif
  }

  void rx_handler() {
    /*
     * Read packet from RX queues
     */
    uint16_t nb_rx = rte_eth_rx_burst(portid, 0, pkts_burst, MAX_PKT_BURST);
    portStatistics.rx += nb_rx;
    for (uint16_t j = 0; j < nb_rx; ++j) {
      struct rte_mbuf *m = pkts_burst[j];
      rte_prefetch0(rte_pktmbuf_mtod(m, void *));
      // l2fwd_simple_forward(m, portid);
    }
  }


  void main_loop() {
    lcore_id_value = rte_lcore_id();
    lcore_id = &lcore_id_value;
    const uint64_t drain_tsc =
        (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
    LOG(INFO) << "entering main loop on lcore " << lcore_id;

    //struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    //struct rte_mbuf *m;
    //uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
    //unsigned i, j, portid, nb_rx;

    // for (unsigned i = 0; i < qconf.n_rx_port; i++) {
    //   portid = qconf.rx_port_list[i];
    //   LOG(INFO) <<  " -- lcoreid=" << lcore_id << " portid=" << portid;
    // }
    uint64_t prev_tsc = 0;
    uint64_t timer_tsc = 0;
    while (1) {
      tx_handler();
      rx_handler();
    }
  }
  void start() { rte_eal_mp_remote_launch(launch, this, CALL_MASTER); }
};

#endif
