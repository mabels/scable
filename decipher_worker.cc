
#include <iomanip>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "rte.h"
#include "decipher_worker.h"

#include "crypto_workers.h"

void DecipherWorker::main_loop() {
  lcore_id = rte_lcore_id();
  LOG(INFO) << "DecipherWorker::main_loop:" << lcore_id;

  struct rte_distributor *distributor = cryptoWorkers.getDistributor();
  struct rte_mbuf *buf;
  while (1) {
    // sleep missing if idle
    buf = rte_distributor_get_pkt(distributor, lcore_id, buf);
    // rte_prefetch0(rte_pktmbuf_mtod(m, void *));
    struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
    LOG(INFO) << "on[" << lcore_id << "] " << eth->s_addr << ">>" << eth->d_addr;
  }
}
