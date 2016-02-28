#ifndef __scable_sockets__
#define __scable_sockets__

#include "rte.h"
#include "socket.h"

class Sockets {
  private:
    std::vector<Socket> sockets;
    Sockets() { }
  public:
    static Sockets *create() {
      auto it = new Sockets();
      int lc;
      RTE_LCORE_FOREACH(lc) {
          auto socketId = rte_lcore_to_socket_id(lc);
          auto socket = std::find_if(it->sockets.begin(),
                            it->sockets.end(),
                            [socketId](const Socket& obj) {
                              return obj.getId() == socketId;
                            });

          if (socket == it->sockets.end()) {
            socket = it->sockets.emplace(it->sockets.end(), Socket(socketId));
          }
          socket->addLcore(lc);
      }
      for(auto socket : it->sockets) {
        LOG(INFO) << "Socket:" << socket.getId()
                  << " Lcore:[" << join(socket.getLcores(), ":") << "]";
      }
      return it;
    }

    int size() const {
      return sockets.size();
    }

    Socket *findFree() {
      for (auto it = sockets.begin(); it != sockets.end(); ++it) {
        if (!it->isFree()) {
          return &(*it);
        }
      }
      LOG(ERROR) << "no free Socket found!";
      return 0;
    }

};

#endif
