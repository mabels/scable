#ifndef __scable_portid__
#define __scable_portid__

#include "rte.h"
class RxWorkers;

class Port {
  public:
    const static uint8_t BURST_TX_DRAIN_US = 100; /* TX drain every ~100us */
    const static uint8_t MAX_PKT_BURST = 32;
    const static uint8_t MAX_RX_QUEUE_PER_LCORE = 16;
    const static uint8_t MAX_TX_QUEUE_PER_PORT = 16;
    const static uint16_t RTE_TEST_RX_DESC_DEFAULT = 128;
    const static uint16_t RTE_TEST_TX_DESC_DEFAULT = 512;
    const static uint16_t CHECK_INTERVAL = 100; /* 100ms */
    const static uint16_t MAX_CHECK_TIME = 90; /* 9s (90 * 100ms) in total */
  private:
    uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
    uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

    struct rte_eth_dev_info dev_info;
    const uint16_t portid;
    const uint8_t rx_queue_per_lcore = 1;
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

    unsigned lcore_id = 0;
  public:
    Port(uint16_t portid)
      : portid(portid) {
      LOG(INFO) << "Created PortId for " << portid;
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

    bool start();
    bool assignLcore2Queue();
    bool checkLinkStatus();

    uint16_t getId() const {
      return portid;
    }

    PortStatistics& getPortStatistics() {
      return portStatistics;
    }
};

#endif
