#include <iomanip>
#include "rte.h"

std::ostream& operator<<(std::ostream &o, const struct ether_addr &ether_addr) {
  o << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[0] << ":"
    << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[1] << ":"
    << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[2] << ":"
    << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[3] << ":"
    << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[4] << ":"
    << std::setw(2) << std::hex << std::setfill('0')
    << (unsigned)ether_addr.addr_bytes[5] << std::dec;
  return o;
}
