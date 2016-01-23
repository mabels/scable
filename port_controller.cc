
#include <iomanip>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "port_controller.h"


bool PortController::initialzePort() {
  LOG(INFO) << "Initializing port " << portid;
  unsigned ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
  if (ret < 0) {
    LOG(ERROR) << "Cannot configure device: err=" << ret
               << ", port=" << portid;
    return false;
  }
  rte_eth_macaddr_get(portid, &ether_addr);
  ret =
      rte_eth_rx_queue_setup(portid, 0, nb_rxd, rte_eth_dev_socket_id(portid),
                             NULL, rtc.getPktMbufPool());
  if (ret < 0) {
    LOG(ERROR) << "rte_eth_rx_queue_setup:err=" << ret << ", port=" << portid;
    return false;
  }
  ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
                               rte_eth_dev_socket_id(portid), NULL);
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
  LOG(INFO) << "Port " << portid << ", MAC address: " << std::setw(2)
            << std::hex << std::setfill('0') << (unsigned)ether_addr.addr_bytes[0]
            << ":" << (unsigned)ether_addr.addr_bytes[1]
            << ":" << (unsigned)ether_addr.addr_bytes[2]
            << ":" << (unsigned)ether_addr.addr_bytes[3]
            << ":" << (unsigned)ether_addr.addr_bytes[4]
            << ":" << (unsigned)ether_addr.addr_bytes[5]
            << std::dec;
}
