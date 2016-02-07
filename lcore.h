#ifndef __scable_lcore__
#define __scable_lcore__

#include "rte.h"

class Lcore {
  private:
    const int id;
    void main_loop() {
        LOG(INFO) << "Starting Lcore on:" << id;
        while (1) {
          
        }
    }
  public:
    static int launch(void *dummy) {
      static_cast<Lcore *>(dummy)->main_loop();
      return 0;
    }

    Lcore(int id) : id(id) {
    }

    bool isMaster() {
      return id == rte_get_master_lcore();
    }

};

#endif
