#include "../../commons/types.h"
#include "../../commons/constants.h"
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

constexpr const char* DATA_DIR = "./data";
constexpr const char* DB_PATH  = "./data/engine.db";
constexpr const char* LOG_PATH = "./data/engine.log";

class StorageManager {

  /*
   * It is a design choice to decide whether these are responsibilities of the storage manager.
  Result<PageId, AllocateErr> AllocatePage();
  Result<OperationStatus, DeallocateErr> DeallocatePage(PageId pid);
  */
  private:
  void RefreshNewPageOffsetIndex();
  void SetNewPageOffsetIndex(uint16_t new_offset);
  uint16_t new_page_offset_index;
  int fd_database;
  int fd_logs;
  public:
  bool Bootstrap();
  ~StorageManager();
  uint16_t GetNewPageOffsetIndex();
  Result<bool> ReadPage(PageID pid, Byte* buffer);
  Result<bool> WritePage(PageID pid, const Byte* buffer);
  Result<PageID> AllocateNewPage();
};
