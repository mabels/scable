#ifndef __scable_action_ref__
#define __scable_action_ref__

class Lcore;

typedef struct S_Action {
  void (*action)(void *, Lcore &lcore);
  void *context;
  const char *name;
} Action;

template<class T>
class ActionRef {
private:
  Action action;
  static void actionFunc(void *ctx, Lcore &lcore) {
      static_cast<T*>(ctx)->action(lcore);
  }
public:
  ActionRef(void *ctx) {
    action.action = ActionRef<T>::actionFunc;
    action.context = ctx;
    action.name = static_cast<T*>(ctx)->name();
  }
  const Action& get() const {
    return action;
  }

};

#endif
