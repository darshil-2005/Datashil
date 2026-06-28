#include <csignal>
#include <atomic>
#include <string>

#include "./include/server/server.h"

std::atomic<bool> server_running{true};
void handle_sigint(int signum) {
    server_running = false;
};

int main(int argc, char* argv[]) {

  if (argc < 2) return 1;
  std::string DB_PATH(argv[1]);
  std::string portno_str;
  if (argc >= 3) portno_str = argv[2];
  else portno_str = "";
  int portno = 8080;

  try {
    portno = std::stoi(portno_str);
  } catch (...) {
    std::cout << "[Main] Invalid Port Number. Using the default portno: " << portno << "\n";
  };

  struct sigaction sa = {};
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  std::signal(SIGPIPE, SIG_IGN);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  Server serv;
  if (serv.Bootstrap(DB_PATH) != 0) {
    std::cerr << "[Main] Bootstrap failed, exiting.\n";
    return 1;
  } else {
    std::cout << "[Main] Bootstrap success.\n";
  };

  serv.RunServer(8080);

  return 0;
}
