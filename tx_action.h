#ifndef __scable_tx_action__
#define __scable_tx_action__

#include "lcore_action.h"

class TXAction {
  private:
    Port *port;
    LcoreActionDelegate<TXAction> actionDelegate;
    TXAction() : actionDelegate(this) {
    }
  public:
    static TXAction *create() {
      return new TXAction();
    }

    TXAction& bindWorker(Cipher2UncipherWorker &c) {
      return *this;
    }

    TXAction& bindWorker(Uncipher2CipherWorker &c) {
      return *this;
    }

    TXAction& bindPort(Port &p) {
      port = &p;
      return *this;
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
      return "TXAction";
    }

    std::string toString() const {
      return port->toString();
    }

};

#endif
