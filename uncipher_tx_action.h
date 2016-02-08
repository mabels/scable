#ifndef __scable_uncipher_tx_action__
#define __scable_uncipher_tx_action__

#include "action_ref.h"

class UncipherTXAction {
  private:
    Port *port;
    ActionRef<UncipherTXAction> actionRef;
  public:
    UncipherTXAction() : actionRef(this) {
    }

    void bindPort(Port *port) {
      this->port = port;
    }

    void action(Lcore &lcore) {
    }

    const Action& getAction() const {
      return actionRef.get();
    }

    const char *name() const {
      return "UncipherTXAction";
    }

};

#endif
