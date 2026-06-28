#include "../include/cli.h"

CLI::CLI(std::atomic<bool> &interrupt_flag) : client_running(interrupt_flag) {};

std::vector<std::string> CLI::ParseResponsePayload(std::vector<Byte> &payload) {

  std::vector<std::string> parsed_payload;
  size_t consumed = 0;
  if (consumed + sizeof(Key) <= payload.size()) {
    Key key;
    memcpy(&key, payload.data(), sizeof(Key));
    consumed += sizeof(Key);
    parsed_payload.push_back(std::to_string(key));
  };

  if (consumed <= payload.size()) {
    std::string response(reinterpret_cast<char*>(payload.data() + sizeof(Key)), payload.size() - sizeof(Key));
    parsed_payload.push_back(response);
  };

  return parsed_payload;
};

void CLI::UseCLI(Client &client) {
  while (client_running) {
    std::string line;
    std::cout << "datashil> ";
    if (!std::getline(std::cin, line)) {
      if (!client_running) {
        std::cout << "[Client] Connection interrupted, closing client.\n";
        break;
      } else {
        // Unlock the cin after interrupt
        std::cin.clear();
        // Clear line string
        line.clear();
        // Clear the half read line before inturrupt from cin buffer 
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      };
    } else {
      Parser query(line);
      query.ParseQuery();
      if (!query.query_object.valid) {
        std::cout << "Invalid Query\n"; 
        line.clear();
        continue;
      };
      try {
        Response query_response = client.SendCommandAndFetchReply(query.query_object);
        std::cout << "Status Code: " << static_cast<int>(query_response.header.status_code) << std::endl;
        if (query_response.header.status_code == 1) {
          std::vector<std::string> parsed_payload = CLI::ParseResponsePayload(query_response.payload);
          std::cout << "===== RESPONSE ======" << std::endl;
          for (int i=1; i<=parsed_payload.size(); i++) {
            std::cout << i << ": " << parsed_payload[i-1] << std::endl;
          };
        } else {
          std::cout << "Query failed!" << std::endl;
        }
      } catch (const std::exception &err) {
        std::cout << "[CLI] Error: " << err.what();
        return;
      };
    };
  };
};
