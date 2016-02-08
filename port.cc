

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "port.h"
#include "ports.h"
#include "rx_workers.h"

bool Port::start() {

  LOG(INFO) << "Initializing port " << portid;


  unsigned ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
  if (ret < 0) {
    LOG(ERROR) << "Cannot configure device: err=" << ret << ", port=" << portid;
    return false;
  }
  rte_eth_macaddr_get(portid, &ether_addr);
  ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd, rte_eth_dev_socket_id(portid),
                               NULL, ports.getPktMbufPool());
  if (ret < 0) {
    LOG(ERROR) << "rte_eth_rx_queue_setup:err=" << ret << ", port=" << portid;
    return false;
  }
  ret = rte_eth_tx_queue_setup(portid, 0, nb_txd, rte_eth_dev_socket_id(portid),
                               NULL);
  if (ret < 0) {
    LOG(ERROR) << "rte_eth_tx_queue_setup:err=" << ret << ", port=" << portid;
    return false;
  }
  /* Start device */
  ret = rte_eth_dev_start(portid);
  if (ret < 0) {
    LOG(ERROR) << "rte_eth_dev_start:err=" << ret << ", port=" << portid;
    return false;
  }
  rte_eth_promiscuous_enable(portid);
  LOG(INFO) << "Port " << portid << ", MAC address: " << ether_addr;
}

bool Port::assignLcore2Queue() {
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

bool Port::checkLinkStatus() {
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
