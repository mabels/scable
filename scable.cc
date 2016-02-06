// #include <iostream>
// #include <iomanip>
// #include <string>
// #include <list>
// #include <vector>
// #include <thread>
// #include <memory>
//
// #include <sys/socket.h>
// #include <unistd.h>


#define ELPP_THREAD_SAFE
#include "easylogging++.h"

//#include "blocking_queue.h"
//#include "rte.h"
#include "rte_controller.h"

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  RteController rteController;
  rteController.start(argc, argv);
}
