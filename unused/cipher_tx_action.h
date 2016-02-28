#ifndef __scable_cipher_tx_action__
#define __scable_cipher_tx_action__

#include "lcore_action.h"

class CipherTXAction {
  private:
    Port *port;
    const LcoreActionDelegate<CipherTXAction> actionDelegate;
  public:
    CipherTXAction() : actionDelegate(this) {
    }

    void bindPort(Port *port) {
      this->port = port;
    }
    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this << "=>" << name();
    }
    void lcoreAction(Lcore &lcore) {
    }

    const LcoreAction& getAction() const {
      return actionDelegate.get();
    }
    const char *name() const {
      return "CipherTXAction";
    }
};

#endif
