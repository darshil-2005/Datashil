#pragma once
#include "../../../../commons/types.h"
#include "../../../../commons/constants.h"
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>


class StorageManager {

  /*
   * It is a design choice to decide whether these are responsibilities of the storage manager.
  Result<PageId, AllocateErr> AllocatePage();
  Result<OperationStatus, DeallocateErr> DeallocatePage(PageId pid);
  */

  private:
    int fd_database;
    int fd_logs;
    void SetNewPageOffsetIndex(uint64_t new_offset);
    uint64_t new_page_offset_index;
    Result<bool> PrivateReadPage(PageID pid, Byte* buffer);
    Result<bool> PrivateWritePage(PageID pid, const Byte* buffer);
    bool IsDatabaseFile(int file_descriptor);

  public:
    bool Bootstrap(std::string DB_PATH);
    ~StorageManager();
    Result<bool> ReadPage(PageID pid, Byte* buffer);
    Result<bool> WritePage(PageID pid, const Byte* buffer);
    Result<PageID> AllocateNewPage();

    // Exposed for testing only
    uint64_t GetNewPageOffsetIndex();
};
