
#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "tx_workers.h"
#include "port.h"

// TxWorkers *TxWorkers::addPort(uint16_t portid) {
//   auto it = ports.find(portid);
//   if (it == ports.end()) {
//     Ports::iterator hint = ports.lower_bound(portid);
//     it = ports.insert(hint, Ports::value_type(portid, Port(portid)));
//     it->second.start();
//   }
//   auto rxWorker = new TxWorker(*this, it->second);
//   workers.push_back(std::unique_ptr<TxWorker>(rxWorker));
//   int ret = rtc.launch(TxWorker::launch, rxWorker);
//   if (ret != 0) {
//     LOG(ERROR) << "rte_eal_remote_launch failed with errno=" << ret;
//     return 0; // UGLY
//   }
//   return this;
// }
