#ifndef __scable_ports__
#define __scable_ports__

#include "rte.h"
#include "port.h"

class Ports {
  private:
    typedef std::map<uint16_t, Port> PortsMap;
    PortsMap ports;
    struct rte_mempool *pktmbufPool;
    Ports() {
    }
    Rte::Name mbufName;
  public:
    static const int NB_MBUF = 8192;
    static Ports *create() {
            auto ports = new Ports();
            /* create the mbuf pool */
            ports->pktmbufPool = rte_pktmbuf_pool_create(ports->mbufName("port:mbuf:"),
                                                   NB_MBUF, 32, 0,
                                                   RTE_MBUF_DEFAULT_BUF_SIZE,
                                                   rte_socket_id());
            if (ports->pktmbufPool == 0) {
                    delete ports;
                    LOG(ERROR) << "Cannot init mbuf pool";
                    return 0;
            }

            uint8_t portCount = rte_eth_dev_count();
            if (portCount == 0) {
              LOG(ERROR) << "No ports defined you need exact two";
              return 0;
            }
            if (portCount != 2) {
              LOG(ERROR) << "not the right # of ports defined you need exact two";
              return 0;
            }
            for (int i = 0; i < portCount; ++i) {
              ports->addPort(i);
            }
            return ports;
    }

    struct rte_mempool *getPktMbufPool() const {
      return pktmbufPool;
    }

    Port *find(int portId) {
            auto it = ports.find(portId);
            if (it == ports.end()) {
              return 0;
            }
            return &(it->second);
    }

    void addPort(int portId) {
            auto it = ports.find(portId);
            if (it == ports.end()) {
                    PortsMap::iterator hint = ports.lower_bound(portId);
                    it = ports.insert(hint, PortsMap::value_type(portId, Port(portId, *this)));
                    it->second.start();
                    LOG(INFO) << "Started Port with id:" << it->second.getId();
            } else {
                    LOG(WARNING) << "Try to add the same port twice!";
            }
    }
};

#endif
