// TODO: Right now forces the caller to pass a bool atomic flag, but implementing multithread makes this better.
//
// NOTE: This problem is called cooperative cancellation and many languages do it exactly like this so this is not a bad solution.
// NOTE: The client has to pass atomic flag and define signal handlers for handling client on interrupts.

#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <exception>
#include <atomic>
#include <limits>

#include "../../../commons/constants.h"
#include "../../../commons/types.h"
#include "../../../commons/src/include/utils/utils.h"

class Client {
  private:
  int sockfd, portno;
  struct sockaddr_in serv_addr;
  void ReadExactlyNBytes(int fd, std::vector<Byte> &buffer, size_t n);
  void WriteExactlyNBytes(int fd, const Byte* buffer, size_t n);
  std::atomic<bool> &client_running;

  public:
  Client(std::string hostname, int portno, std::atomic<bool> &interrupt_signal);
  ~Client();
  Response SendCommandAndFetchReply(Query query);
  // void Stop();
};
