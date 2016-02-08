#ifndef __scable_lcores__
#define __scable_lcores__

#include "rte.h"
#include "lcore.h"

class Lcores {
  private:
    std::vector<std::unique_ptr<Lcore>> lcores;
  public:
    bool addLcore(int id) {
      lcores.push_back(std::unique_ptr<Lcore>(new Lcore(id)));
    }

    int size() const {
      return lcores.size();
    }

    Lcore* findFree() {
      for (auto it = lcores.begin() ; it != lcores.end(); ++it) {
        if (!(*it)->hasActions()) {
          return it->get();
        }
      }
      LOG(ERROR) << "no free Lcore found!";
      return 0;
    }

    bool launch() {
      Lcore *master = 0;
      int id = 0;
      for (auto it = lcores.begin() ; it != lcores.end(); ++it, ++id) {
        if ((*it)->isMaster()) {
          master = it->get();
        } else {
          rte_eal_remote_launch(Lcore::launch, it->get(), id);
        }
      }
      Lcore::launch(master);
      return false;
    }
};

#endif
