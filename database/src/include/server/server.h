#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <atomic>
#include <exception>

#include "../../commons/types.h"
#include "../../../../commons/src/include/utils/utils.h"
#include "../../include/storageManager/storageManager.h"
#include "../../include/bufferpool/bufferpool.h"
#include "../../include/b-tree/b-tree.h"

class Server {
  private:
    int sockfd, newsockfd;
    std::unique_ptr<StorageManager> storage_manager;
    std::unique_ptr<BufferPool> bufferpool;
    std::unique_ptr<BTree> btree;

    void HandleClient();
    void HandleSearch(std::vector<Byte> &stream_buffer);
    void HandleInsert(std::vector<Byte> &stream_buffer);
    void HandleDelete(std::vector<Byte> &stream_buffer);
    void WriteExactlyNBytes(int fd, const Byte* data, size_t n);
    void ReadExactlyNBytes(int fd, std::vector<Byte> &buffer, size_t n);

  public:
    int RunServer(int portno);
    int Bootstrap(std::string DB_PATH);
    Server();
    ~Server();
};

