
#include <../../commons/constants.h>
#include <../../commons/types.h>
#include <../storageManger/storageManager.h>

// 5 Bytes
struct BufferFrameMeta {
  PageID page_id = -1;
  uint16_t pin_count = 0;
  bool is_dirty = false;
};

class BufferPool {
  public:  
  BufferPool(StorageManager& sm);
  Result<Byte*> RequestPage(PageID pid);
  Result<bool> ReleasePage(PageID pid, bool is_dirty);

  
  private:
  StorageManager* storage_manager;

  Byte buffer_pool[POOL_SIZE * PAGE_SIZE];
  BufferFrameMeta meta_buffer_pool[POOL_SIZE];
  Result<bool> SetPageInDisk(PageID pid, const Byte* buffer);
  // Return Err no free page found
  Result<OffsetIndex> CheckFreeSpace();
  Result<OffsetIndex> FindPageToEvict();
  Result<bool> EvictPage(OffsetIndex idx);
};
