#ifndef __scable_uncipher_2_cipher__
#define __scable_uncipher_2_cipher__

#include "lcore_action.h"
#include "pkt_action.h"
#include "cipher_controller.h"

class Uncipher2CipherWorker {
  private:
    const PktActionDelegate<Uncipher2CipherWorker> uncipherActionDelegate;
    CipherController &cipherController;
    std::vector<RXAction *> rxActions;
    std::vector<TXAction *> txActions;
    Uncipher2CipherWorker(CipherController &cipherController)
      : uncipherActionDelegate(this),
        cipherController(cipherController) {
    }
  public:
    static Uncipher2CipherWorker *create(CipherController &cipherController) {
      return new Uncipher2CipherWorker(cipherController);
    }

    void pktPrepare() {
      LOG(INFO) << "Starting PktAction "
        << "for RXActions:[" << join(rxActions, ",") << "]:"
        << "for TXActions:[" << join(txActions, ",") << "]:"
        << this << "=>" << name();
    }

    bool pktAction(struct rte_mbuf *buf) {
      if (rxActions.end() != std::find_if(rxActions.begin(),
                        rxActions.end(),
                        [buf](const RXAction *obj) {
                          return obj->getPort()->getId() == buf->port;
                        })) {
        struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
        LOG(INFO) << "on[" << name() << buf->port << "] " << eth->s_addr << ">>" << eth->d_addr;
        return true;
      }
      return false;
    }

    const PktAction& getCipherAction() const {
      return uncipherActionDelegate.get();
    }

    const char *name() const {
      return "Uncipher2CipherWorker";
    }

    // struct rte_ring *getDistributorRing() const {
    //   return distributorRing;
    // }

    Uncipher2CipherWorker& bindRXAction(RXAction &rxa) {
      rxActions.push_back(&rxa);
      return *this;
    }
    Uncipher2CipherWorker& bindTXAction(TXAction &txa) {
      txActions.push_back(&txa);
      return *this;
    }


};

#endif
