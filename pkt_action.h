#ifndef __scable_pkt_action__
#define __scable_pkt_action__

#include "rte.h"
#include "port.h"

typedef struct S_PktAction {
  void (*prepare)(void *);
  bool (*action)(void *, struct rte_mbuf *buf);
  Port *(*getport)(void *);
  void *context;
} PktAction;

template<class T>
class PktActionDelegate {
private:
  PktAction action;
  static void prepareFunc(void *ctx) {
      return static_cast<T*>(ctx)->pktPrepare();
  }
  static bool actionFunc(void *ctx, struct rte_mbuf *buf) {
      return static_cast<T*>(ctx)->pktAction(buf);
  }
  // static Port* getportFunc(void *ctx) {
  //     return static_cast<T*>(ctx)->getPort();
  // }
public:
  PktActionDelegate(void *ctx) {
    action.prepare = PktActionDelegate<T>::prepareFunc;
    action.action = PktActionDelegate<T>::actionFunc;
    // action.getport = PktActionDelegate<T>::getportFunc;
    action.context = ctx;
  }
  const PktAction& get() const {
    return action;
  }

};

#endif
