#include <../../commons/types.h>
#include <../../commons/constants.h>

class StorageManager {
  /*
   * It is a design choice to decide whether these are responsibilities of the storage manager.
  Result<PageId, AllocateErr> AllocatePage();
  Result<OperationStatus, DeallocateErr> DeallocatePage(PageId pid);
  */
  bool Bootstrap();
  ~StorageManager();
  Result<bool> ReadPage(PageId pid, Byte* buffer);
  Result<bool> WritePage(PageId pid, const Byte* buffer);
  private:
  int fd_database;
  int fd_logs;
};
