#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "rte_controller.h"
#include "rx_workers.h"

bool RteController::start(int argc, char **argv) {
  int ret = rte.eal_init(argc, argv);
  if (ret < 0) {
    LOG(ERROR) << "Invalid EAL arguments";
    return false;
  }
  /* create the mbuf pool */
  pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool",
                              NB_MBUF, 32, 0,
                              RTE_MBUF_DEFAULT_BUF_SIZE,
                              rte_socket_id());
  if (pktmbuf_pool == NULL) {
    LOG(ERROR) << "Cannot init mbuf pool";
    return false;
  }
  uint8_t ports = rte_eth_dev_count();
  if (ports == 0) {
    LOG(ERROR) << "No ports defined you need exact two";
    return false;
  }
  if (ports != 2) {
    LOG(ERROR) << "not the right # of ports defined you need exact two";
    return false;
  }

  int lc;
  usedLcores.resize(rte_lcore_count());
  LOG(INFO) << "Setup Lcore Used bitmap:" << rte_lcore_count() << ":" << usedLcores.size();
  LOG(INFO) << "Lcore Masterid:" << rte_get_master_lcore();
  RTE_LCORE_FOREACH(lc) {
      usedLcores.push_back(false);
  }
  usedLcores[rte_get_master_lcore()] = true;

  const int numCryptWorkers = 2;
  std::unique_ptr<CryptoWorkers> cryptoWorkers(new CryptoWorkers(*this, numCryptWorkers));

  std::unique_ptr<RxWorkers> rxWorkers((new RxWorkers(*this, *cryptoWorkers))->addPort(0));
  //std::unique_ptr<CryptWorkers> cryptController(CryptWorkers.start(this, 2));

  // for (uint8_t portid = 0; portid < ports; ++portid) {
  //   std::unique_ptr<PortController> my(new PortController(*this, portid));
  //   if (!my->assignLcore2Queue()) {
  //     return false;
  //   }
  //   my->checkLinkStatus();
  //   my->initialzePort();
  //   my->start();
  //   this->ports.push_back(std::move(my));
  // }
  rte_eal_mp_wait_lcore();
  // bool stop = false;
  // while (!stop) {
  //   rte_eal_mp_wait_lcore	(	void 		)
  //
  //   rxWorkers.isRunning();
  //   cryptWorkers.isRunning();
  //    for (auto it = this->ports.begin() ; it != this->ports.end(); ++it) {
  //      unsigned *val = (*it)->getLcoreId();
  //      if (val && rte_eal_wait_lcore(*val) < 0) {
  //      stop = true;
  //        LOG(INFO) << "Stopped lcoreId" << *val;
  //        break;
  //      }
  //    }
  // }
  return true;
}

bool RteController::launch(lcore_function_t *func, void *arg) {
  int id = 0;
  for (std::vector<bool>::iterator it = usedLcores.begin() ; it != usedLcores.end(); ++it, ++id) {
    if (!*it) {
      *it = true;
      rte_eal_remote_launch(func, arg, id);
      return false;
    }
  }
  return false;
}
