#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "rte_controller.h"
// #include "rx_workers.h"
// #include "tx_workers.h"
// #include "crypto_workers.h"
// #include "decipher_worker.h"
// #include "cipher_worker.h"
#include "ports.h"
#include "sockets.h"
#include "rx_action.h"
#include "tx_action.h"

#include "distribution_controller.h"
#include "cipher_controller.h"
#include "cipher_2_uncipher_worker.h"
#include "uncipher_2_cipher_worker.h"


RXAction& operator>>(Port &p, RXAction &rxa) {
  return rxa.bindPort(p.bindRXAction(rxa));
}

Uncipher2CipherWorker& operator>>(RXAction &rxa, Uncipher2CipherWorker &c) {
  return c.bindRXAction(rxa.bindWorker(c));
}

Cipher2UncipherWorker& operator>>(RXAction &rxa, Cipher2UncipherWorker &c) {
  return c.bindRXAction(rxa.bindWorker(c));
}

TXAction& operator>>(Uncipher2CipherWorker &c, TXAction &txa) {
  return txa.bindWorker(c.bindTXAction(txa));
}

TXAction& operator>>(Cipher2UncipherWorker &c, TXAction &txa) {
  return txa.bindWorker(c.bindTXAction(txa));
}

void operator>>(TXAction &txa, Port &p) {
  p.bindTXAction(txa.bindPort(p));
}



bool RteController::start(int argc, char **argv) {
  int ret = rte.eal_init(argc, argv);
  if (ret < 0) {
    LOG(ERROR) << "Invalid EAL arguments";
    return false;
  }
  auto sockets = std::unique_ptr<Sockets>(Sockets::create());

  /*
   *     distributionController
   *     cipherController
   *         uncipher2cipher
   *         cipher2unciper
   *     uncipherRXAction
   *     cipherRXAction
   *     uncipherTXAction
   *     cipherTXAction
   */

  auto ports = std::unique_ptr<Ports>(Ports::create());

  auto socket = sockets->findFree();

  auto lcorePacket = socket->findFree();
  auto lcoreDistributionController = lcorePacket;
  auto lcoreUncipherRXAction = lcorePacket;
  auto lcoreCipherRXAction = lcorePacket;
  auto lcoreUncipherTXAction = lcorePacket;
  auto lcoreCipherTXAction = lcorePacket;

  /*
   *     uncipher RX(portX) -> dc -> workerx(portX) -> dc -> TX(portY)
   *     cipher RX(portY) -> dc -> workery(portY) -> dc -> TX(portX)
   */

  auto distributionController = std::unique_ptr<DistributionController>(DistributionController::create());
  lcoreDistributionController->addAction(distributionController->getAction());

  auto cipherController = std::unique_ptr<CipherController>(CipherController::create(*(distributionController.get())));
  auto uncipher2cipherWorker = std::unique_ptr<Uncipher2CipherWorker>(Uncipher2CipherWorker::create(*(cipherController.get())));
  auto cipher2uncipherWorker = std::unique_ptr<Cipher2UncipherWorker>(Cipher2UncipherWorker::create(*(cipherController.get())));

  for(auto lcore = socket->findFree(); lcore != 0; lcore = socket->findFree()) {
    lcore->addAction(cipherController->getAction());
  }

  auto uncipherRXAction = std::unique_ptr<RXAction>(RXAction::create());
  lcoreUncipherRXAction->addAction(uncipherRXAction->getAction());

  auto cipherTXAction = std::unique_ptr<TXAction>(TXAction::create());
  lcoreCipherTXAction->addAction(cipherTXAction->getAction());

  *(ports->find(0)) >>
    *uncipherRXAction >>
    *uncipher2cipherWorker >>
    *cipherTXAction >>
    *(ports->find(1));


  // auto cipherTXAction = std::unique_ptr<CipherTXAction>(CipherTXAction::create(*(distributionController.get())));
  // cipherTXAction->bindPort(ports->find(1));
  // lcoreCipherTXAction->addAction(cipherTXAction->getAction());

  auto cipherRXAction = std::unique_ptr<RXAction>(RXAction::create());
  lcoreCipherRXAction->addAction(cipherRXAction->getAction());

  auto uncipherTXAction = std::unique_ptr<TXAction>(TXAction::create());
  lcoreUncipherTXAction->addAction(uncipherTXAction->getAction());

  *(ports->find(1)) >>
    *cipherRXAction >>
    *cipher2uncipherWorker >>
    *uncipherTXAction >>
    *(ports->find(0));

  // auto uncipherTXAction = std::unique_ptr<UncipherTXAction>(UncipherTXAction::create(*(distributionController.get())));
  // uncipherTXAction->bindPort(ports->find(0));
  // lcoreUncipherTXAction->addAction(uncipherTXAction->getAction());


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
  socket->launch();
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
