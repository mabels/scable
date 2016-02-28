#ifndef __scable_lcore__
#define __scable_lcore__

#include <string>

#include "rte.h"

#include "lcore_action.h"

class Socket;

class Lcore {
  private:
    const int id;
    std::vector<LcoreAction> actions;
    Socket &socket;
  public:
    static int launch(void *dummy) {
      static_cast<Lcore *>(dummy)->main_loop();
      return 0;
    }

    Lcore(Socket &socket, int id)
      : socket(socket)
      , id(id) {
      LOG(INFO) << "Lcore::Lcore:" << id << ":" << this;
    }

    int getId() const {
      return id;
    }

    void addAction(const LcoreAction &action) {
      LOG(INFO) << "Lcore::addAction:" << id << ":" << action.context << ":" << this;
      actions.push_back(action);
    }

    bool hasActions() const {
      return actions.size() != 0;
    }

    bool isMaster() const {
      return id == rte_get_master_lcore();
    }

    std::string toString() const {
        return std::to_string(id);
    }
  private:
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

};

#endif
