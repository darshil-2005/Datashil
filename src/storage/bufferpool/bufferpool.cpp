#include <./bufferpool.h>


BufferPool::BufferPool(StorageManager& sm_input) {
  this->storage_manager = &sm_input;
};

Result<OffsetIndex> BufferPool::CheckFreeSpace() {

  ssize_t index = -1;
  for (int i=0; i<POOL_SIZE; i++) {
    if (meta_buffer_pool[i].page_id == -1) {
      index = i;
      break;
    };
  };
  if (index == -1) {
    return { .value = 0, .err = ErrType::BufferPoolFull };
  };

  return { .value = index, .err = ErrType::None };
};

// TODO: Figure out how to handle all pages are pinned.
Result<Byte*> BufferPool::RequestPage(PageId pid) {
  ssize_t index = -1;
  for (int i=0; i<POOL_SIZE; i++) {
    if (meta_buffer_pool[i].page_id == pid) {
      index = i;
      break;
    };
  };

  if (index != -1) {
    Offset off = PAGE_SIZE * index;
    meta_buffer_pool[index].pin_count++;
    return { .value = buffer_pool + off, .err = ErrType::None };
  };

  Result<OffsetIndex> free_space = CheckFreeSpace();

  if (free_space.err != ErrType::None) {
    // No free space
    Result<OffsetIndex> evict_page_index = FindPageToEvict();
    Result<OffsetIndex> evicted_index = EvictPage(evict_page_index.value);
    // Handle errors here if any
    if (evicted_index.err != ErrType::AllPagesPinned) {
      return { .value = nullptr, .err = evicted_index.err };      
    };
    index = evicted_index.value;
  } else {
    index = free_space.value;
  };

  Offset off = PAGE_SIZE * index;
  Result<bool> storage_read_status = storage_manager->ReadPage(pid, buffer_pool + off); 

  // Maybe we can handle this hear rather than passing it upwards
  if (storage_read_status.value == false) {
    return { .value = nullptr, .err = storage_read_status.err };
  };

  meta_buffer_pool[index].pin_count = 1;
  meta_buffer_pool[index].page_id = pid;
  meta_buffer_pool[index].is_dirty = false;

  return { .value = buffer_pool + off, .err = ErrType::None };
};
