#ifndef __scable_cipher_2_uncipher_worker__
#define __scable_cipher_2_uncipher_worker__

#include "lcore_action.h"
#include "pkt_action.h"
#include "cipher_controller.h"

class Cipher2UncipherWorker {
  private:
    std::vector<Port *> ports;
    const PktActionDelegate<Cipher2UncipherWorker> uncipherActionDelegate;
    CipherController &cipherController;
    std::vector<RXAction *> rxActions;
    std::vector<TXAction *> txActions;
    Cipher2UncipherWorker(CipherController &cipherController)
      : uncipherActionDelegate(this),
        cipherController(cipherController) {
    }
  public:
    static Cipher2UncipherWorker *create(CipherController &cipherController) {
      return new Cipher2UncipherWorker(cipherController);
    }

    Port *bindPort(Port *port) {
      ports.push_back(port);
      return port;
    }

    void pktPrepare() {
      LOG(INFO) << "Starting PktAction "
        << "for RXActions:[" << join(rxActions, ",") << "]:"
        << "for TXActions:[" << join(txActions, ",") << "]:"
        << this << "=>" << name();
    }

    bool pktAction(struct rte_mbuf *buf) {
      if (ports.end() != std::find_if(ports.begin(),
                        ports.end(),
                        [buf](const Port *obj) {
                          return obj->getId() == buf->port;
                        })) {
        //if (buf->port == port->getId()) {
        struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
        LOG(INFO) << "on[" << name() << buf->port << "] "
                  << eth->s_addr << ">>" << eth->d_addr;
        return true;
      }
      return false;
    }

    const PktAction& getCipherAction() const {
      return uncipherActionDelegate.get();
    }

    // Port *getPort() const {
    //   return port;
    // }

    const char *name() const {
      return "Cipher2UncipherWorker";
    }

    Cipher2UncipherWorker& bindRXAction(RXAction &rxa) {
      rxActions.push_back(&rxa);
      return *this;
    }
    Cipher2UncipherWorker& bindTXAction(TXAction &txa) {
      txActions.push_back(&txa);
      return *this;
    }

};

#endif
