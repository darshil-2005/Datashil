#pragma once
#include <atomic>

#include "./parser.h"
#include "./client.h"

class CLI {
  private:
    std::atomic<bool> &client_running;
    std::vector<std::string> ParseResponsePayload(std::vector<Byte> &payload);

  public:
    CLI(std::atomic<bool> &interrupt_flag);
    void UseCLI(Client &client);
};
