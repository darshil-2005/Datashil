#include <atomic>
#include <stdexcept>
#include <string>
#include <csignal>

#include "./include/client.h"
#include "./include/cli.h"

std::atomic<bool> client_running{true};
void handle_sigint(int signum) {
    client_running = false;
};

int main(int argc, char* argv[]) {

  if (argc < 3) return 1;
  std::string HOSTNAME(argv[1]);
  std::string PORT(argv[2]);
  int portno;

  try {
    portno = std::stoi(PORT);
  } catch (...) {
    throw std::runtime_error("[Main] Please enter a valid port number.");
  };

  struct sigaction sa = {};
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  std::signal(SIGPIPE, SIG_IGN);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  Client client(HOSTNAME, portno, client_running);

  CLI cli(client_running);
  cli.UseCLI(client);

  return 0;
};
