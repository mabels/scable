// class Packet {
//   private:
//     const int size;
//     std::unique_ptr<unsigned char> packet;
//   public:
//     typedef Packet *Ptr;
//     Packet(int _size) : size(_size), packet(new unsigned char[_size]) {
//     }
//     int getSize() const {
//       return size;
//     }
//     unsigned char *getSize() {
//       return packet.get();
//     }
// };
//
// class InputProcessor {
//   private:
//     std::thread th;
//     int socket;
//     std::string name;
//     bool running;
//     BlockingQueue<std::shared_ptr<Packet>> packetQ;
//   public:
//     ~InputProcessor() {
//       th.join();
//       close(socket);
//     }
//     bool start(const std::string& _name, int mtu, int queueSize) {
//       for (int i = 0; i < queueSize; ++i) {
//         packetQ.push(std::shared_ptr<Packet>(new Packet(mtu)));
//       }
//       name = _name;
//       socket = socket(PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
//       if (socket < 0) {
//         LOG(ERROR) << "can't open raw socket:" << errno;
//         return false;
//       }
//       if (setsockopt(socket, SOL_SOCKET, SO_BINDTODEVICE, name.c_str(),
//       name.length) < 0) {
//         LOG(ERROR) << "cannot bind to interface:" << name << ":" << errno;
//         close(socket);
//       }
//       struct ifreq ifr;
//       strncpy((char*)ifr.ifr_name, _name.c_str(), IF_NAMESIZE);
//       if (ioctl(socket, SIOCGIFINDEX, &ifr)<0) {
//         LOG(ERROR) << "cannot get interface index:" << name << ":" << errno;
//         close(socket);
//       }
//       ifr.ifr_flags |= IFF_PROMISC;
//       if( ioctl(socket, SIOCSIFFLAGS, &ifr) != 0 ) {
//         LOG(ERROR) << "cannot set interface to promisc mode:" << name << ":"
//         << errno;
//         close(socket);
//       }
//       running = true;
//       th = std::thread(&InputProcessor::run, this);
//       return true;
//     }
//     void run() {
//       LOG(INFO) << "InputProcessor started for interface:" << name;
//       while (running) {
//         std::shared_ptr<Packet> packet = packetQ.pop();
//         struct sockaddr from;
//         socklen_t fromlen = sizeof(from);
//         int cnt = recvfrom(socket, packet->getData(), packet->getSize(), 0,
//         (struct sockaddr *) &from, &fromlen)
//         LOG(DEBUG) << cnt << "=recvfrom(" << socket << ")";
//         packetQ.push(packet);
//       }
//     }
// };
//
// class InputManager {
//   private:
//     const std::list<InputProcessor> processors;
//   public:
//       void add_if(const std::string &if_name, int mtu, int qsize) {
//         InputProcessor ip;
//         if (!ip.start(if_name, mtu, qsize)) {
//           LOG(ERROR) << "can't start InputProcessor for " << if_name;
//           return;
//         }
//         processors.push_back(ip)
//       }
// };
