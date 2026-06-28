#include "../include/client.h"
#include <cstddef>

class ClientShutdownException: public std::exception {
  public:
    const char* what() const noexcept override {
      return "Client shutting down via interrupt.\n";
    };
};

class CorruptPacketException : public std::exception {
  public:
    const char* what() const noexcept override {
      return "Received corrupt response from server.\n";
    };
};

Client::Client(std::string hostname, int portno, std::atomic<bool> &interrupt_flag) : client_running(interrupt_flag) {

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    throw std::runtime_error("Socket creation error.\n");
  };
  this->portno = portno;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  if (inet_pton(AF_INET, hostname.c_str(), &serv_addr.sin_addr) <= 0) {
    throw std::runtime_error("[Client] Invalid address or Address not supported.\n");
  };
  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    throw std::runtime_error("[Client] Connection to server failed.\n");
  };

  std::cout << "[Client] Connection success, Ready to go.\n";
};

void Client::ReadExactlyNBytes(int fd, std::vector<Byte> &buffer, size_t n) {
  size_t total_read = 0;
  Byte temp_buffer[1024];
  while (total_read < n) {
    size_t bytes_to_read = std::min(n - total_read, sizeof(temp_buffer));
    ssize_t bytes_read = read(fd, temp_buffer, bytes_to_read);

    if (bytes_read == 0) {
      throw std::runtime_error(std::string("[Client] Connection closed prematurely, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    if (bytes_read < 0) {
      if (errno == EINTR) {
        if (!client_running) {
          std::cout << "[Client] Caller interrupted, shuting down client.\n";
          throw ClientShutdownException();
        };
        continue;
      };
      if (errno == ECONNRESET) {
        throw std::runtime_error("[Client] Connection reset by peer.");
      }
      throw std::runtime_error(std::string("[Client] Error reading from the server, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    total_read += bytes_read;
    buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_read);
  };
};

void Client::WriteExactlyNBytes(int fd, const Byte* buffer, size_t n) {

  size_t count = 0;
  size_t total_written = 0;
  while (total_written < n) {
    ssize_t bytes_written = write(fd, buffer + total_written, n - total_written);

    if (total_written == total_written + bytes_written) {
      count++;
      if (count >= 100) {
        // TODO: Abort the request.
        throw std::runtime_error("[Client] Failing to write to the server again and again, Aborting.");
      };
    } else { 
      count = 0;
    };

    if (bytes_written < 0) {
      if (errno == EINTR) {
        if (!client_running) {
          std::cout << "[Client] Caller interrupted, shuting down client.\n";
          throw ClientShutdownException();
        };
        continue;
      };
      if (errno == EPIPE) {
        throw std::runtime_error("[Client] Server went offline, cannot continue.");
      };
      throw std::runtime_error(std::string("[Client] Error writing to the server, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    total_written += bytes_written;
  };
};

// TODO: Build streaming into and out of the server for payload right now putting everything in memory which is not sustainable for larger payloads.
Response Client::SendCommandAndFetchReply(Query query) {

  Command command = query.command;
  std::vector<std::string> &payload = query.payload; 
  Key key = query.key;

  RequestHeader request_header;
  request_header.command = command;
  request_header.magic_number = NETWORK_MAGIC_NUMBER;
  request_header.total_length = sizeof(RequestHeader) + sizeof(Key);
  for (int i = 0; i < payload.size(); i++) {
    request_header.total_length += payload[i].size();
  };

  Utils::SetChecksum(reinterpret_cast<Byte*>(&request_header), sizeof(RequestHeader));
  Client::WriteExactlyNBytes(sockfd, reinterpret_cast<const Byte*>(&request_header), sizeof(RequestHeader));
  Client::WriteExactlyNBytes(sockfd, reinterpret_cast<const Byte*>(&key), sizeof(Key));

  for (int i=0; i<payload.size(); i++) {
    Client::WriteExactlyNBytes(sockfd, reinterpret_cast<const Byte*>(payload[i].data()), payload[i].size());
  };

  std::vector<Byte> response_header_vector;
  Client::ReadExactlyNBytes(sockfd, response_header_vector, sizeof(ResponseHeader));
  ResponseHeader response_header;
  memcpy(&response_header, response_header_vector.data(), sizeof(ResponseHeader));

  if (response_header.magic_number != NETWORK_MAGIC_NUMBER || response_header.echo_command != request_header.command) {
    return Response();
  };

  if (!Utils::VerifyChecksum(reinterpret_cast<const Byte*>(&response_header), sizeof(ResponseHeader))) {
    return Response();
  };

  // TODO: Dont read this all in one go read 1024 then send and then read again.
  size_t bytes_to_read = response_header.total_length - sizeof(ResponseHeader);
  std::vector<Byte> payload_buffer;
  Client::ReadExactlyNBytes(sockfd, payload_buffer, bytes_to_read);

  return Response(response_header, std::move(payload_buffer));
};

Client::~Client() {
  if (sockfd != -1) {
    close(sockfd);
    sockfd = -1;
  };
};
