// TODO: Can add trailing checksum for the whole payload later.

#include "../../include/server/server.h"
#include <cstddef>
#include <stdexcept>

class CorruptPacketException : public std::exception {
  public:
    const char* what() const noexcept override {
      return "Received corrupt response from client.\n";
    };
};

class ServerShutdownException : public std::exception {
  public:
    const char* what() const noexcept override {
      return "Server shutting down via interrupt.\n";
    };
};

extern std::atomic<bool> server_running;

Server::Server() {
  sockfd = -1, newsockfd = -1;
};

Server::~Server() {
  if (sockfd != -1) {
    close(sockfd);
  };
  if (newsockfd != -1) {
    close(newsockfd);
  };
};

void Server::WriteExactlyNBytes(int fd, const Byte* buffer, size_t n) {

  size_t count = 0;
  size_t total_written = 0;
  while (total_written < n) {
    ssize_t bytes_written = write(fd, buffer + total_written, n - total_written);

    if (total_written == total_written + bytes_written) {
      count++;
      if (count >= 100) {
        // TODO: Tell the client there request failed.
        throw std::runtime_error("[Server] Failing to write to the client again and again, Aborting.");
      };
    } else { 
      count = 0;
    };

    if (bytes_written < 0) {
      if (errno == EINTR) {
        if (!server_running) {
          throw ServerShutdownException();
        };
        continue;
      };
      if (errno == EPIPE) {
        throw std::runtime_error("[Server] Client disconnected abruptly (Broken Pipe).");
      };
      throw std::runtime_error(std::string("[Server] Error writing to the client, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    total_written += bytes_written;
  };
};

void Server::ReadExactlyNBytes(int fd, std::vector<Byte> &buffer, size_t n) {
  size_t total_read = 0;
  Byte temp_buffer[1024];

  while (total_read < n) {
    size_t bytes_to_read = std::min(n - total_read, sizeof(temp_buffer));
    ssize_t bytes_read = read(fd, temp_buffer, bytes_to_read);
    if (bytes_read == 0) {
      // TODO: Maybe this can be handled without closing the server entierly.
      throw std::runtime_error(std::string("[Server] Connection closed prematurely, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    if (bytes_read < 0) {
      if (errno == EINTR) {
        if (!server_running) {
          throw ServerShutdownException();
        };
        continue;
      };
      if (errno == ECONNRESET) {
        throw std::runtime_error("[Server] Connection reset by peer.");
      }
      throw std::runtime_error(std::string("[Server] Error reading from the client, Error code: ") + std::to_string(errno) + "\n Error description: " + strerror(errno));
    };
    buffer.insert(buffer.end(), temp_buffer, temp_buffer + bytes_read);
    total_read += bytes_read;
  };
};

void Server::HandleSearch(std::vector<Byte> &stream_buffer){
  RequestHeader request_header;
  memcpy(&request_header, stream_buffer.data(), sizeof(RequestHeader));
  // Assuming key will be constant size for now
  Key search_key; 
  memcpy(&search_key, reinterpret_cast<Byte*>(stream_buffer.data()) + sizeof(RequestHeader), sizeof(Key));
  Byte buffer[1024];

  PayloadStream search_result = btree->Search(search_key);
  uint64_t payload_size = static_cast<uint64_t>(search_result.PayloadRemaining());
  
  ResponseHeader response_header;
  response_header.magic_number = NETWORK_MAGIC_NUMBER;
  response_header.echo_command = Command::Search;
  if (search_result.IsEOF()) {
    response_header.status_code = 4;
  } else {
    response_header.status_code = 1;
  };
  response_header.total_length = sizeof(ResponseHeader) + payload_size;
  Utils::SetChecksum(reinterpret_cast<Byte*>(&response_header), sizeof(ResponseHeader));

  Server::WriteExactlyNBytes(newsockfd, reinterpret_cast<const Byte*>(&response_header), sizeof(ResponseHeader));

  while (!search_result.IsEOF()) {
    size_t bytes_read = search_result.NextBytes(buffer, sizeof(buffer));
    Server::WriteExactlyNBytes(newsockfd, buffer, bytes_read); 
  };

  stream_buffer.erase(stream_buffer.begin(), stream_buffer.begin() + request_header.total_length);
};

void Server::HandleInsert(std::vector<Byte> &stream_buffer) {
  RequestHeader request_header;
  memcpy(&request_header, stream_buffer.data(), sizeof(RequestHeader));

  Key insert_key;
  memcpy(&insert_key, stream_buffer.data() + sizeof(RequestHeader), sizeof(Key));
  if (request_header.total_length > 20000 || request_header.total_length < sizeof(RequestHeader)) {
    // corrupt packet
    throw CorruptPacketException();
  };
  uint64_t payload_size = request_header.total_length - sizeof(RequestHeader);
  bool insert_result = btree->Insert(stream_buffer.data() + sizeof(RequestHeader), payload_size, insert_key);

  ResponseHeader response_header;
  response_header.total_length = sizeof(ResponseHeader) + sizeof(Key);
  response_header.magic_number = NETWORK_MAGIC_NUMBER;
  if (insert_result) {
    response_header.status_code = 1;
  } else {
    response_header.status_code = 0;
  };
  response_header.echo_command = Command::Insert;
  Utils::SetChecksum(reinterpret_cast<Byte*>(&response_header), sizeof(ResponseHeader));
  Server::WriteExactlyNBytes(newsockfd, reinterpret_cast<const Byte*>(&response_header), sizeof(ResponseHeader));
  Server::WriteExactlyNBytes(newsockfd, reinterpret_cast<const Byte*>(&insert_key), sizeof(Key));

  stream_buffer.erase(stream_buffer.begin(), stream_buffer.begin() + request_header.total_length);
};

void Server::HandleDelete(std::vector<Byte> &stream_buffer) {
  RequestHeader request_header;
  memcpy(&request_header, stream_buffer.data(), sizeof(RequestHeader));
  // Assuming key will be constant size for now
  Key delete_key; 
  memcpy(&delete_key, stream_buffer.data() + sizeof(RequestHeader), sizeof(Key));
  bool delete_result = btree->Delete(delete_key);
  
  ResponseHeader response_header;
  response_header.magic_number = NETWORK_MAGIC_NUMBER;
  response_header.echo_command = Command::Delete;
  if (delete_result) {
    response_header.status_code = 1;
  } else {
    response_header.status_code = 4;
  };
  response_header.total_length = sizeof(ResponseHeader) + sizeof(Key);
  Utils::SetChecksum(reinterpret_cast<Byte*>(&response_header), sizeof(ResponseHeader));

  Server::WriteExactlyNBytes(newsockfd, reinterpret_cast<const Byte*>(&response_header), sizeof(ResponseHeader));
  Server::WriteExactlyNBytes(newsockfd, reinterpret_cast<const Byte*>(&delete_key), sizeof(Key));

  stream_buffer.erase(stream_buffer.begin(), stream_buffer.begin() + request_header.total_length);
};

void Server::HandleClient() {

  Byte buffer[1024];
  std::vector<Byte> stream_buffer;

  while (server_running) {
    ssize_t n = read(newsockfd, buffer, sizeof(buffer));
    if (n == 0) {
      std::cout << "[Server] Client closed connection gracefully" << std::endl;
      break;
    };

    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    };

    stream_buffer.insert(stream_buffer.end(), buffer, buffer + n);

    while (stream_buffer.size() >= sizeof(RequestHeader)) {
      RequestHeader request_header;
      memcpy(&request_header, stream_buffer.data(), sizeof(RequestHeader));
      if (request_header.magic_number != NETWORK_MAGIC_NUMBER) {
        std::cout << "[Server] Connection corrupted! Magic number does not match." << std::endl;
        return;
      };

      // Check the header checksum here and nuke the server if the checksum does not match.
      if (!Utils::VerifyChecksum(reinterpret_cast<const Byte*>(&request_header), sizeof(RequestHeader))) {
        std::cout << "[Server] Connection corrupted! Checksum does not match." << std::endl;
        return;
      };

      // Check the size of the stream buffer again and if it is less than request_header break the loop.
      if (stream_buffer.size() < request_header.total_length) break;

      switch (request_header.command) {
        case Command::Search:
          try {
            Server::HandleSearch(stream_buffer);
          } catch(const ServerShutdownException &err) {
            std::cout << "[Server] " << err.what() << std::endl;
            return;
          } catch(const std::exception& err) {
            std::cout << "[Server] Error searching, Error description: " << err.what() << std::endl;
            return;
          };
          break;
        case Command::Insert:
          try {
            Server::HandleInsert(stream_buffer);
          } catch(const ServerShutdownException &err) {
            std::cout << "[Server] " << err.what() << std::endl;
            return;
          } catch(const std::exception &err) {
            std::cout << "[Server] Error inserting, Error description: " << err.what() << std::endl;
            return;
          };
          break;
        case Command::Delete:
          try {
            Server::HandleDelete(stream_buffer);
          } catch(const ServerShutdownException &err) {
            std::cout << "[Server] " << err.what() << std::endl;
            return;
          } catch(const std::exception &err) {
            std::cout << "[Server] Error deleting, Error description: " << err.what() << std::endl;
            return;
          };
          break;
        case Command::Quit:
          std::cout << "[Server] Gracefully closing the connection" << std::endl;
          return;
        default:
          throw std::runtime_error("[Server] Invalid comand type.");
      };
    };
  };
};

int Server::Bootstrap(std::string DB_PATH) {

  std::cout << "[Server] Starting Up." << std::endl;

  std::cout << "[Server] Launching Storage Manager." << std::endl;
  storage_manager = std::make_unique<StorageManager>();

  if (storage_manager->Bootstrap(DB_PATH)) {
    std::cout << "[Server] Storage Manager bootstrap successful." << std::endl;
  } else {
    return 1;
  };

  std::cout << "[Server] Launching Bufferpool." << std::endl;
  try {
    bufferpool = std::make_unique<BufferPool>(*storage_manager);
  } catch (const std::exception& err) {
    std::cout << "[Server] Failed to launch bufferpool, details below: \n" << err.what() << std::endl;
    return 1;
  };
  std::cout << "[Server] Bufferpool launch success." << std::endl;

  std::cout << "[Server] Initializing BTree." << std::endl;

  try {
    btree = std::make_unique<BTree>(*bufferpool);
  } catch(const std::exception& err) {
    std::cout << "[Server] Failed to initialize BTree, details below: \n" << err.what() << std::endl;
    return 1;
  };

  return 0;
};

int Server::RunServer(int portno) {
  int n;
  struct sockaddr_in serv_addr, cli_addr;

  Byte buffer[1024];
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (sockfd < 0) {
    throw std::runtime_error("[Server] Failed to get the socket.");
  };

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
    close(sockfd);
    throw std::runtime_error(std::string("[Server] Failed to bind socket to the port, error code: ") + std::to_string(errno) + "\n" + std::string("Error description: ") + strerror(errno));
  };

  listen(sockfd, 5);

  while (server_running) {
    socklen_t chilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &chilen);
    if (newsockfd < 0) {
      if (errno == EINTR) {
        continue;
      };
      throw std::runtime_error("Accept failed");
    };
    Server::HandleClient();
  };

  return 0;
};
