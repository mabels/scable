#ifndef __scable_lcores__
#define __scable_lcores__

#include "rte.h"
#include "lcore.h"

class Lcores {
  private:
    std::vector<Lcore> lcores;
  public:
    bool addLcore(int id) {
      Lcore lcore(id);
      lcores.push_back(lcore);
    }
    bool launch() {
      Lcore *master = 0;
      int id = 0;
      for (std::vector<Lcore>::iterator it = lcores.begin() ; it != lcores.end(); ++it, ++id) {
        if (it->isMaster()) {
          master = &*it;
        } else {
          rte_eal_remote_launch(Lcore::launch, &*it, id);
        }
      }
      Lcore::launch(&master);
      return false;
    }
};

#endif
