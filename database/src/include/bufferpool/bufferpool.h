
#pragma once
#include "../../../../commons/constants.h"
#include "../../../../commons/types.h"
#include "../storageManager/storageManager.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <stdlib.h>
#include <iostream>

// 5 Bytes
struct __attribute__((__packed__)) BufferFrameMeta {
  PageID page_id = 0;
  uint16_t pin_count = 0;
  bool is_dirty = false;
  uint8_t reference_bit = 0;
  bool is_in_queue = false;
};

class BufferPool {
  public:  
  BufferPool(StorageManager& sm);
  ~BufferPool();
  Result<Byte*> RequestPage(PageID pid);
  Result<bool> ReleasePage(PageID pid, bool is_dirty);
  Result<NewPage> AllocateNewPage();
  int GetTotalPinnedPages();

  private:
  Byte* buffer_pool;
  StorageManager* storage_manager;
  std::unordered_map<PageID, OffsetIndex> page_table;
  std::vector<OffsetIndex> free_frames;
  std::queue<OffsetIndex> unpinned_frames;
  BufferFrameMeta buffer_pool_meta[POOL_SIZE];

  // Return Err no free page found
  Result<OffsetIndex> CheckFreeSpace();
  Result<OffsetIndex> FindPageToEvict();
  Result<OffsetIndex> EvictPage(OffsetIndex idx);
};
