#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "rte_controller.h"
// #include "rx_workers.h"
// #include "tx_workers.h"
// #include "crypto_workers.h"
// #include "decipher_worker.h"
// #include "cipher_worker.h"
#include "uncipher_rx_action.h"
#include "uncipher_tx_action.h"

#include "cipher_rx_action.h"
#include "cipher_tx_action.h"

bool RteController::start(int argc, char **argv) {
  int ret = rte.eal_init(argc, argv);
  if (ret < 0) {
    LOG(ERROR) << "Invalid EAL arguments";
    return false;
  }
  uint8_t portCount = rte_eth_dev_count();
  if (portCount == 0) {
    LOG(ERROR) << "No ports defined you need exact two";
    return false;
  }
  if (portCount != 2) {
    LOG(ERROR) << "not the right # of ports defined you need exact two";
    return false;
  }
  ports = std::unique_ptr<Ports>(Ports::create());
  for (int i = 0; i < portCount; ++i) {
    ports->addPort(i);
  }
  int lc;
  RTE_LCORE_FOREACH(lc) {
      lcores.addLcore(lc);
  }

  auto cipherController = std::unique_ptr<CipherController>(CipherController::create(lcores.size()));
  /*
   *  UncipherRXAction(Port0)(lc0) -> CiperAction(lc2,lc3)    -> CipherTXAction(Port1)(lc1)
   */
  auto lcoreRxCore = lcores.findFree();
  auto uncipherRXAction = std::unique_ptr<UncipherRXAction>(new UncipherRXAction(*(cipherController.get())));
  uncipherRXAction->bindPort(ports->find(0));
  cipherController->addCipherAction(uncipherRXAction->getCipherAction());
  lcoreRxCore->addAction(uncipherRXAction->getAction());

  auto lcoreTxCore = lcores.findFree();
  auto cipherTXAction = std::unique_ptr<CipherTXAction>(new CipherTXAction());
  cipherTXAction->bindPort(ports->find(1));
  lcoreTxCore->addAction(cipherTXAction->getAction());

  /*
   *  CipherRXAction(Port1)(lc0)   -> UncipherAction(lc2,lc3) -> UncipherTXAction(Port0)(lc1)
   */
  auto cipherRXAction = std::unique_ptr<CipherRXAction>(new CipherRXAction(*(cipherController.get())));
  cipherRXAction->bindPort(ports->find(1));
  cipherController->addCipherAction(cipherRXAction->getCipherAction());
  lcoreRxCore->addAction(cipherRXAction->getAction());

  auto uncipherTXAction = std::unique_ptr<UncipherTXAction>(new UncipherTXAction());
  uncipherTXAction->bindPort(ports->find(0));
  lcoreTxCore->addAction(uncipherTXAction->getAction());

  for(auto lcore = lcores.findFree(); lcore != 0; lcore = lcores.findFree()) {
    lcore->addAction(cipherController->getAction());
  }

//  CipherAction ciperAction = new CipherAction();
//  DecipherAction deciperAction = new DecipherAction();


  // const int numCryptWorkers = 1;
  //
  // std::unique_ptr<CryptoWorkers> cipherWorkers(CryptoWorkers::create(*this, CipherWorker::factory, numCryptWorkers));
  // std::unique_ptr<CryptoWorkers> decipherWorkers(CryptoWorkers::create(*this, DecipherWorker::factory, numCryptWorkers));
  //
  // std::unique_ptr<RxWorkers> rxWorkers((new RxWorkers(*this, *cipherWorkers))->joinPort(0));
  // std::unique_ptr<TxWorkers> cipherTxWorkers((new TxWorkers(*this, *cipherWorkers))->joinPort(1));
  //
  // std::unique_ptr<RxWorkers> decipherRxWorkers((new RxWorkers(*this, *decipherWorkers))->joinPort(1));
  // std::unique_ptr<TxWorkers> txWorkers((new TxWorkers(*this, *decipherWorkers))->joinPort(0));
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
  lcores.launch();
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
