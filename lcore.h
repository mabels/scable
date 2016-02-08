#ifndef __scable_lcore__
#define __scable_lcore__

#include "rte.h"
#include "action_ref.h"

class Lcore {
  private:
    const int id;
    void main_loop() {
      if (!hasActions()) {
        LOG(INFO) << "Young kill Lcore on:" << id << ":" << this;
        return;
      }
      for(auto it = actionRefs.begin(); it != actionRefs.end(); ++it) {
        LOG(INFO) << "Starting Lcore on:" << id << ":" << this << "=>" << it->name;
      }
      while (hasActions()) {
        for(auto it = actionRefs.begin(); it != actionRefs.end(); ++it) {
          (*(it->action))(it->context, *this);
        }
      }
    }
    std::vector<Action> actionRefs;
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

    void addAction(const Action &action) {
      LOG(INFO) << "Lcore::addAction:" << id << ":" << action.context << ":" << this;
      actionRefs.push_back(action);
    }

    bool hasActions() {
      return actionRefs.size() != 0;
    }

    bool isMaster() {
      return id == rte_get_master_lcore();
    }

};

#endif
