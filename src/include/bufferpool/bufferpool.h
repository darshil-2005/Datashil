
#include <../../commons/constants.h>
#include <../../commons/types.h>
#include <../storageManger/storageManager.h>
#include <unordered_map>
#include <stdlib.h>

// 5 Bytes
struct BufferFrameMeta {
  PageID page_id = -1;
  uint16_t pin_count = 0;
  bool is_dirty = false;
  uint8_t reference_bit = 0;
};

class BufferPool {
  public:  
  BufferPool(StorageManager& sm);
  ~BufferPool();
  Result<Byte*> RequestPage(PageID pid);
  Result<bool> ReleasePage(PageID pid, bool is_dirty);
  Result<NewPage> AllocateNewPage();
  
  private:
  StorageManager* storage_manager;
  unordered_map<PageID, OffsetIndex> page_table;
  vector<OffsetIndex> free_frames;
  queue<OffsetIndex> unpinned_frames;
  Byte* buffer_pool;
  BufferFrameMeta buffer_pool_meta[POOL_SIZE];

  // Return Err no free page found
  Result<OffsetIndex> CheckFreeSpace();
  Result<OffsetIndex> FindPageToEvict();
  Result<OffsetIndex> EvictPage(OffsetIndex idx);
};
