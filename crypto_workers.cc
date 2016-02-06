cryptWorkers = rte_distributor_create("cryptWorkers", rte_socket_id(), numCryptWorkers);
if (cryptWorkers == NULL) {
  LOG(ERROR) << "Cannot create distributor";
  return false;
}
