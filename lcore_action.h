#ifndef __scable_lcore_action__
#define __scable_lcore_action__

class Lcore;

typedef struct S_LcoreAction {
  void (*prepare)(void *, Lcore &lcore);
  void (*action)(void *, Lcore &lcore);
  void *context;
} LcoreAction;

template<class T>
class LcoreActionDelegate {
private:
  LcoreAction action;
  static void prepareFunc(void *ctx, Lcore &lcore) {
      static_cast<T*>(ctx)->lcorePrepare(lcore);
  }
  static void actionFunc(void *ctx, Lcore &lcore) {
      static_cast<T*>(ctx)->lcoreAction(lcore);
  }
public:
  LcoreActionDelegate(void *ctx) {
    action.prepare = LcoreActionDelegate<T>::prepareFunc;
    action.action  = LcoreActionDelegate<T>::actionFunc;
    action.context = ctx;
  }
  const LcoreAction& get() const {
    return action;
  }

};

#endif
