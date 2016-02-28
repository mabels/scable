#ifndef __scable_socket__
#define __scable_socket__

#include "rte.h"
#include "lcore.h"

class Socket {
  private:
    std::vector<Lcore> lcores;
    const unsigned socketId;
  public:
    Socket(const Socket &socket)
      : socketId(socket.socketId)
      , lcores(socket.lcores) {
    }

    Socket(const unsigned socketId) : socketId(socketId) { }

    // static Socket *create(const unsigned socketid) {
    //   return new Socket(socketid);
    // }

    unsigned getId() const {
      return socketId;
    }

    bool addLcore(int id) {
      lcores.push_back(Lcore(*this, id));
    }

    const std::vector<Lcore>& getLcores() const {
      return lcores;
    }

    int size() const {
      return lcores.size();
    }

    bool isFree() const {
      bool ret = true;
      for (auto it : lcores) {
        ret &= !it.hasActions();
      }
      return ret;
    }

    Lcore* findFree() {
      for (auto it = lcores.begin() ; it != lcores.end(); ++it) {
        if (!(it->hasActions())) {
          return &(*it);
        }
      }
      LOG(ERROR) << "no free Lcore found!";
      return 0;
    }

    bool launch() {
      Lcore *master = 0;
      for (auto it = lcores.begin() ; it != lcores.end(); ++it) {
        if (it->isMaster()) {
          master = &(*it);
        } else {
          rte_eal_remote_launch(Lcore::launch, &(*it), it->getId());
        }
      }
      Lcore::launch(master);
      return false;
    }
};

#endif
