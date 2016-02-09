#ifndef __scable_uncipher_tx_action__
#define __scable_uncipher_tx_action__

#include "lcore_action.h"

class UncipherTXAction {
  private:
    Port *port;
    LcoreActionDelegate<UncipherTXAction> actionDelegate;
  public:
    UncipherTXAction() : actionDelegate(this) {
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
      return "UncipherTXAction";
    }

};

#endif
