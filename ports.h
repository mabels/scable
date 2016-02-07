#ifndef __scable_ports_
#define __scable_ports__

#include "rte.h"
#include "port.h"

class Ports {
  private:
    typedef std::map<uint16_t, Port> PortsMap;
    PortsMap ports;
  public:
    void addPort(int portId) {
      auto it = ports.find(portId);
      if (it == ports.end()) {
        PortsMap::iterator hint = ports.lower_bound(portId);
        it = ports.insert(hint, PortsMap::value_type(portId, Port(portId)));
        it->second.start();
        LOG(INFO) << "Started Port with id:" << it->second.getId();
      } else {
        LOG(WARNING) << "Try to add the same port twice!";
      }
    }
};

#endif
