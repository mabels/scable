#ifndef __scable_lcore__
#define __scable_lcore__

#include "rte.h"
#include "lcore_action.h"

class Lcore {
  private:
    const int id;
    void main_loop() {
      if (!hasActions()) {
        LOG(INFO) << "Young kill Lcore on:" << id << ":" << this;
        return;
      }
      for(auto it = actions.begin(); it != actions.end(); ++it) {
        (*(it->prepare))(it->context, *this);
        //LOG(INFO) << "Starting Lcore on:" << id << ":" << this << "=>" << it->name;
      }
      while (1) {
        for(auto it = actions.begin(); it != actions.end(); ++it) {
          (*(it->action))(it->context, *this);
        }
      }
    }
    std::vector<LcoreAction> actions;
  public:
    static int launch(void *dummy) {
      static_cast<Lcore *>(dummy)->main_loop();
      return 0;
    }

    Lcore(int id) : id(id) {
      LOG(INFO) << "Lcore::Lcore:" << id << ":" << this;
    }

    int getId() const {
      return id;
    }

    void addAction(const LcoreAction &action) {
      LOG(INFO) << "Lcore::addAction:" << id << ":" << action.context << ":" << this;
      actions.push_back(action);
    }

    bool hasActions() {
      return actions.size() != 0;
    }

    bool isMaster() {
      return id == rte_get_master_lcore();
    }

};

#endif
