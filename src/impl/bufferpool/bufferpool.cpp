#include <./bufferpool.h>

BufferPool::BufferPool(StorageManager &sm_input) {
  this->storage_manager = &sm_input;

  int aligned =
      posix_memalign((Byte **)&buffer_pool, PAGE_SIZE, POOL_SIZE * PAGE_SIZE);

  if (aligned == ENOMEM) {
    std::cout << "[FATAL ERROR] Cannot allocate aligned memory to buffer pool."
              << std::endl;
    std::exit(1);
  };

  for (int i = 0; i < POOL_SIZE; i++) {
    free_frames.push_back(i);
  };
};

BufferPool::~BufferPool() {
  if (buffer_pool != nullptr) {
    free(buffer_pool);
  };
};

Result<OffsetIndex> BufferPool::FindPageToEvict() {
  ssize_t index = -1;

  size_t MAX_ITERS = 2 * unpinned_frames.size();
  for (int i = 0; i < MAX_ITERS; i++) {
    index = unpinned_frames.front();
    unpinned_frames.pop();
    BufferFrameMeta &frame_meta = buffer_pool_meta[index];
    if (frame_meta.pin_count > 0) {
      continue;
    };
    if (frame_meta.reference_bit == 1) {
      frame_meta.reference_bit = 0;
      unpinned_frames.push(index);
      continue;
    };
    break;
  };

  if (index == -1) {
    return {.value = 0, .err = ErrType::AllPagesPinned};
  };
  return {.value = index, .err = ErrType::None};
};

Result<OffsetIndex> BufferPool::EvictPage(OffsetIndex index) {
  BufferFrameMeta &frame_meta = buffer_pool_meta[index];

  // Might add functionality for trying again instead of crashing
  if (frame_meta.is_dirty) {
    PageID pid = frame_meta.page_id;
    Offset offset = PAGE_SIZE * index;
    Result<bool> write_status =
        storage_manager->WritePage(pid, buffer_pool + offset);
    if (write_status.err != ErrType::None) {
      return {.value = 0, .err = write_status.err};
    };
    frame_meta.is_dirty = false;
  };

  page_table.erase(frame_meta.page_id);
  frame_meta = (struct BufferFrameMeta){
      .page_id = -1,
      .pin_count = 0,
      .is_dirty = false,
      .reference_bit = 0,
  };
  return {.value = index, .err = ErrType::None};
};

Result<OffsetIndex> BufferPool::CheckFreeSpace() {

  if (free_frames.empty()) {
    return {.value = 0, .err = ErrType::AllPagesPinned};
  };

  OffsetIndex index = free_frames.back();
  return {.value = index, .err = ErrType::None};
};

Result<Byte *> BufferPool::RequestPage(PageID pid) {
  ssize_t index = -1;
  if (page_table.count(pid)) {
    index = page_table[pid];
  };

  if (index != -1) {
    Offset off = PAGE_SIZE * index;
    buffer_pool_meta[index].pin_count++;
    buffer_pool_meta[index].reference_bit = 1;
    return {.value = buffer_pool + off, .err = ErrType::None};
  };

  Result<OffsetIndex> free_space = CheckFreeSpace();

  if (free_space.err != ErrType::None) {
    // No free space
    Result<OffsetIndex> page_to_evict = FindPageToEvict();
    if (page_to_evict.err != ErrType::None) {
      return {.value = nullptr, .err = ErrType::BufferPoolFull};
    };
    Result<OffsetIndex> evicted_index = EvictPage(page_to_evict.value);
    if (evicted_index.err != ErrType::None) {
      return {.value = nullptr, .err = ErrType::BufferPoolFull};
    };
    // Handle errors here if any
    index = evicted_index.value;
  } else {
    index = free_space.value;
    // Probably bad design for multi threaded architecture but fine for my
    // implementation.
    free_frames.pop_back();
  };

  Offset offset = PAGE_SIZE * index;
  Result<bool> storage_read_status =
      storage_manager->ReadPage(pid, buffer_pool + offset);

  // Maybe we can handle this hear rather than passing it upwards
  if (storage_read_status.value == false) {
    return {.value = nullptr, .err = storage_read_status.err};
  };

  buffer_pool_meta[index].pin_count = 1;
  buffer_pool_meta[index].page_id = pid;
  buffer_pool_meta[index].is_dirty = false;
  buffer_pool_meta[index].reference_bit = 1;
  page_table[pid] = index;

  return {.value = buffer_pool + offset, .err = ErrType::None};
};

Result<bool> ReleasePage(PageID pid, bool is_dirty) {
  if (!page_table.count(pid)) {
    return {.value = false, .err = ErrType::PageNotFoundInBufferPool};
  };

  OffsetIndex index = page_table[pid];
  BufferFrameMeta &frame_meta = buffer_pool_meta[index];
  frame_meta.pin_count--;
  if (frame_meta.pin_count == 0) {
    unpinned_frames.push_back(index);
  };
  frame_meta[index].is_dirty = true;
  return {.val = true, .err = ErrType::None};
};

Result<NewPage> AllocateNewPage(){
  Result<PageID> result = storage_manager->AllocateNewPage();
  // handle errors
  Result<Byte*> ptr_to_page = RequestPage(result.value);
  return { .value = { .value = { .pid = result.value, .ptr = ptr_to_page }, .err = ErrType::None }, .err = ErrType::None }
};


