#ifndef __scable_rte__
#define __scable_rte__

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_distributor.h>

#include <iostream>
#include <iomanip>

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

class Rte {
public:
  class Name {
    private:
      std::ostringstream name;
    public:
      const char *operator()(const char *base) {
        name << base << std::hex << this;
        LOG(INFO) << "Rte::Name:" << name.str();
        return name.str().c_str();
      }
  };
  int eal_init(int argc, char **argv) { return rte_eal_init(argc, argv); }
  void exit() { rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n"); }
};

std::ostream& operator<<(std::ostream &o, const struct ether_addr &ether_addr);

template <typename C, typename T>
class ToStringJoiner {
  C &c;
  T &s;
  ToStringJoiner(C &&container, T&& sep)
    : c(std::forward<C>(container))
    , s(std::forward<T>(sep)) {
  }
public:
  template <typename CC, typename TT> friend std::ostream& operator<<(std::ostream &o, ToStringJoiner<CC, TT> const &mj);
  template <typename CC, typename TT> friend ToStringJoiner<CC, TT> join(CC &&container, TT&& sep);
};

template<typename T> T * ptr(T & obj) { return &obj; } //turn reference into pointer!

template<typename T> T * ptr(T * obj) { return obj; } //obj is already pointer, return it!

template<typename C, typename T> std::ostream& operator<<(std::ostream &o, ToStringJoiner<C, T> const &mj) {
  const char *sep = "";
  for (auto i : mj.c) {
        o << sep;
        o << ptr(i)->toString();
        sep = mj.s;
  }
  return o;
}

template<typename C, typename T> ToStringJoiner<C, T> join(C &&container, T&& sep) {
    return ToStringJoiner<C, T>(std::forward<C>(container), std::forward<T>(sep));
}

#endif
