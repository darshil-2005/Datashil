#include "../../include/page/leaf_page.h"

// need to handle both leaf and overflow page
uint16_t LeafPage::CheckAvailableSpace(Byte* page) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  Byte* free_space_start = page + LEAF_PAGE_HEADER_SIZE + (page_header->slot_array_size * SLOT_SIZE);
  Byte* free_space_end = page + page_header->free_space_end_offset;

  uint16_t free_bytes = free_space_end - free_space_start + 1;
  return free_bytes;
};

uint16_t LeafPage::CheckGarbageBytes(Byte* page) {
 return reinterpret_cast<LeafPageHeader*>(page)->garbage_bytes;
};

SlotArrayElement* LeafPage::upper_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x) {

  int16_t n = end - start;
  int16_t l = 0;
  int16_t r = n - 1;
  int16_t ans = n;

  while (l <= r) {
    int16_t mid = l + (r - l) / 2;
    int16_t original_mid = mid;
    SlotArrayElement* element = start + mid;
    while (element->is_deleted > 0) {
      mid++;
      element = start + mid;
      if (element == end) {
        r = original_mid - 1;
        break;
      };
    };

    if (element == end) {
      continue;
    };

    Byte* tuple = page + element->offset;
    Key* key = reinterpret_cast<Key*>(tuple + TUPLE_HEADER_SIZE); 
    if (*key <= x) {
      l = mid + 1;
    } else {
      ans = mid;
      r = original_mid - 1;
    };
  };

  return start + ans;
};

Key LeafPage::HandleSplit(Byte* old_page, Byte* new_page, PageID new_pid) {

  Byte buffer[PAGE_SIZE];
  memcpy(buffer, old_page, PAGE_SIZE);

  uint16_t threshold = (PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / 2;
  uint16_t consumed_space = 0;

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(buffer);
  SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(buffer + LEAF_PAGE_HEADER_SIZE);
  SlotArrayElement* slot_array_end = slot_array_start + page_header->slot_array_size;
  SlotArrayElement* slot_array_new_page_start = reinterpret_cast<SlotArrayElement*>(buffer + LEAF_PAGE_HEADER_SIZE);

  uint16_t live_count = 0;

  for (int i=0; i < page_header->slot_array_size; i++) {
    SlotArrayElement* current = slot_array_start + i;
    if (current->is_deleted > 0) continue;
    consumed_space = consumed_space + current->length;
    live_count++;
    if (consumed_space > threshold) {
      slot_array_new_page_start = current + 1;      
      while (slot_array_new_page_start != slot_array_end && slot_array_new_page_start->is_deleted > 0) {
        slot_array_new_page_start++;
      };
      break;
    };
  };

  if (live_count == 0) return 0;

  if (slot_array_new_page_start == slot_array_end || slot_array_new_page_start == slot_array_start) {
    slot_array_new_page_start = slot_array_start;
    uint16_t cnt = 0;
    for (int i=0; i<page_header->slot_array_size; i++) {
      if (slot_array_new_page_start->is_deleted > 0) {
        slot_array_new_page_start++; 
        continue;
      };
      cnt++;
      if (cnt > (live_count / 2)) break;
      slot_array_new_page_start++;
    };
  };

  LeafPage::MakePage(old_page, slot_array_start, 
      (uint16_t)(slot_array_new_page_start - slot_array_start),
      buffer, page_header->page_id, page_header->left_pid, new_pid);

  LeafPage::MakePage(new_page, slot_array_new_page_start, 
      (uint16_t)((slot_array_start + page_header->slot_array_size) - slot_array_new_page_start),
      buffer, new_pid, page_header->page_id, page_header->right_pid);

  Key boundary_key = LeafPage::GetKeyFromSlotElement(buffer, slot_array_new_page_start);
  return boundary_key;
};

bool LeafPage::MakePage(Byte* page, SlotArrayElement* slot_array_start, uint16_t slot_array_size, Byte* buffer, PageID pid, PageID left_pid, PageID right_pid) {

  PageOffset free_space_end_offset = PAGE_SIZE - 1;
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  page_header->page_type = PageType::LeafPage;
  page_header->page_id = pid;
  page_header->left_pid = left_pid;
  page_header->right_pid = right_pid;
  page_header->garbage_bytes = 0;

  if (slot_array_size == 0) {
    page_header->slot_array_size = 0;
    page_header->free_space_end_offset = PAGE_SIZE - 1;
    return true;
  };

  SlotArrayElement* slot_array = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);

  uint16_t compact_slot_array_size = 0;
  size_t i=0;
  while (i<slot_array_size) {
    
    if (slot_array_start[i].is_deleted > 0) {
      i++;
      continue;
    };

    PageOffset offset = slot_array_start[i].offset;
    TupleLength length = slot_array_start[i].length;

    memcpy(page + free_space_end_offset - length + 1, buffer + offset, length); 
    slot_array[compact_slot_array_size].offset = free_space_end_offset - length + 1;
    slot_array[compact_slot_array_size].length = length;
    slot_array[compact_slot_array_size].is_deleted = 0;
    free_space_end_offset = slot_array[compact_slot_array_size].offset - 1;
    compact_slot_array_size++;
    i++;
  };

  page_header->slot_array_size = compact_slot_array_size;
  page_header->free_space_end_offset = free_space_end_offset;
  
  return true;
};

SlotArrayElement* LeafPage::lower_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x) {

  int16_t n = end - start;
  int16_t l = 0;
  int16_t r = n - 1;
  int16_t ans = n;

  while (l <= r) {
    int16_t mid = l + (r - l) / 2;
    int16_t original_mid = mid;
    SlotArrayElement* element = start + mid;
    while (element->is_deleted > 0) {
      mid++;
      element = start + mid;
      if (element == end) {
        r = original_mid - 1;
        break;
      };
    };

    if (element == end) {
      continue;
    };

    Byte* tuple = page + element->offset;
    Key* key = reinterpret_cast<Key*>(tuple + TUPLE_HEADER_SIZE); 
    if (*key < x) {
      l = mid + 1;
    } else {
      ans = mid;
      r = original_mid - 1;
    };
  };

  return start + ans;
};

Key LeafPage::GetKeyFromSlotElement(Byte* page, SlotArrayElement* element) {
  Key key;
  Byte* key_address = page + element->offset + TUPLE_HEADER_SIZE;
  memcpy(&key, key_address, sizeof(Key));
  return key;
};

uint32_t LeafPage::GetTupleSizeFromSlotElement(Byte* page, SlotArrayElement* element) {
  TupleHeader* tuple_header = reinterpret_cast<TupleHeader*>(page + element->offset);
  return tuple_header->size;
};

void LeafPage::DumpPageFirstLast(Byte *page) {
  LeafPageHeader *page_header = reinterpret_cast<LeafPageHeader *>(page);

  std::cout << "\n==================================== LEAF PAGE ========================================" << std::endl;
  std::cout << "=============== LEAF PAGE HEADER ===============" << std::endl;
  std::cout << "Page Type: " << static_cast<int>(page_header->page_type)
            << std::endl;
  std::cout << "Page ID: " << page_header->page_id << std::endl;
  std::cout << "Free Space End Offset: " << page_header->free_space_end_offset
            << std::endl;
  std::cout << "Slot Array Size: " << page_header->slot_array_size << std::endl;
  std::cout << "Left Sibling: " << page_header->left_pid << std::endl;
  std::cout << "Right Sibling: " << page_header->right_pid << std::endl;
  std::cout << "Garbage Bytes: " << page_header->garbage_bytes << std::endl;
  std::cout << "================= HEADER END =================\n" << std::endl;  

  SlotArrayElement *slot_array =
      reinterpret_cast<SlotArrayElement *>(page + LEAF_PAGE_HEADER_SIZE);
  for (int i = 0; i < page_header->slot_array_size; i++) {
    OverflowInfo *overflow_info =
        reinterpret_cast<OverflowInfo *>(page + slot_array[i].offset);

    if (slot_array[i].is_deleted > 0) {
      continue;
    } else {
      std::cout << "=========================================" << std::endl;
      std::cout << "Tuple Offset: " << slot_array[i].offset << std::endl;
      std::cout << "Tuple Length: " << slot_array[i].length << std::endl;
      std::cout << "Tuple Overflow: "
                << static_cast<int>(overflow_info->overflow) << std::endl;
      std::cout << "Overflow Page: " << overflow_info->overflow_page
                << std::endl;
      std::cout << "Key: "
                << *reinterpret_cast<uint16_t *>(page + slot_array[i].offset +
                                                 TUPLE_HEADER_SIZE)
                << std::endl;
      break;
      std::cout << "=========================================" << std::endl;
    };
  };
  for (int i = page_header->slot_array_size - 1; i>=0; i--) {
    OverflowInfo *overflow_info =
        reinterpret_cast<OverflowInfo *>(page + slot_array[i].offset);

    if (slot_array[i].is_deleted > 0) {
      continue;
    } else {
      std::cout << "=========================================" << std::endl;
      std::cout << "Tuple Offset: " << slot_array[i].offset << std::endl;
      std::cout << "Tuple Length: " << slot_array[i].length << std::endl;
      std::cout << "Tuple Overflow: "
                << static_cast<int>(overflow_info->overflow) << std::endl;
      std::cout << "Overflow Page: " << overflow_info->overflow_page
                << std::endl;
      std::cout << "Key: "
                << *reinterpret_cast<uint16_t *>(page + slot_array[i].offset +
                                                 TUPLE_HEADER_SIZE)
                << std::endl;
      std::cout << "=========================================" << std::endl;
      break;
    };

  };
  std::cout << "=============== LEAF PAGE END ===============" << std::endl;
};

void LeafPage::DumpPage(Byte *page) {
  LeafPageHeader *page_header = reinterpret_cast<LeafPageHeader *>(page);

  std::cout << "=========================================" << std::endl;
  std::cout << "Page Type: " << static_cast<int>(page_header->page_type)
            << std::endl;
  std::cout << "Page ID: " << page_header->page_id << std::endl;
  std::cout << "Free Space End Offset: " << page_header->free_space_end_offset
            << std::endl;
  std::cout << "Slot Array Size: " << page_header->slot_array_size << std::endl;
  std::cout << "Left Sibling: " << page_header->left_pid << std::endl;
  std::cout << "Right Sibling: " << page_header->right_pid << std::endl;
  std::cout << "Garbage Bytes: " << page_header->garbage_bytes << std::endl;
  std::cout << "=========================================" << std::endl;

  SlotArrayElement *slot_array =
      reinterpret_cast<SlotArrayElement *>(page + LEAF_PAGE_HEADER_SIZE);
  for (int i = 0; i < page_header->slot_array_size; i++) {
    OverflowInfo *overflow_info =
        reinterpret_cast<OverflowInfo *>(page + slot_array[i].offset);

    // if (slot_array[i].is_deleted > 0) {
    //   std::cout << "ELEMENT DELETED: " << i + 1 << std::endl;
    // } else {
    //   std::cout << "Tuple Offset: " << slot_array[i].offset << std::endl;
    //   std::cout << "Tuple Length: " << slot_array[i].length << std::endl;
    //   std::cout << "Tuple Overflow: "
    //             << static_cast<int>(overflow_info->overflow) << std::endl;
    //   std::cout << "Overflow Page: " << overflow_info->overflow_page
    //             << std::endl;
    //   std::cout << "Key: "
    //             << *reinterpret_cast<uint16_t *>(page + slot_array[i].offset +
    //                                              TUPLE_HEADER_SIZE)
    //             << std::endl;
    // };

    if (slot_array[i].is_deleted > 0) {
      std::cout << "D ";
    } else {
      std::cout << *reinterpret_cast<uint16_t *>(page + slot_array[i].offset + TUPLE_HEADER_SIZE) << " ";
    };
  };
  std::cout << "\n=========================================" << std::endl;
};

SearchResult LeafPage::Search(Byte* page, Key key) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
  SlotArrayElement* slot_array_end = slot_array_start + page_header->slot_array_size;
  SlotArrayElement* search_result = LeafPage::lower_bound(slot_array_start, slot_array_end, page, key);

  if (search_result == slot_array_end) {
    return { .found = false, .tuple_offset = 0, .tuple_end_offset = 0, .total_tuple_size = 0, .overflow = false, .overflow_page_id = 0 };
  };

  Key returned_key = LeafPage::GetKeyFromSlotElement(page, search_result);
  TupleHeader* tuple_header = reinterpret_cast<TupleHeader*>(page + search_result->offset);
  size_t tuple_size = tuple_header->size;
  bool overflow = (tuple_header->overflow > 0) ? true : false;
  PageID overflow_page_id = tuple_header->overflow_page;
  

  if (returned_key == key) {
    return { 
      .found = true,
      .tuple_offset = search_result->offset,
      .tuple_end_offset = (Offset)(search_result->offset + search_result->length),
      .total_tuple_size = tuple_size,
      .overflow = overflow, 
      .overflow_page_id = overflow_page_id };
  };

  return { .found = false, .tuple_offset = 0, .tuple_end_offset = 0, .total_tuple_size = 0, .overflow = false, .overflow_page_id = 0 };
};

PayloadStream::PayloadStream() {
  buffer_pool = nullptr;
  curr_pid = 0;
  curr_page_offset = 0;
  curr_page_end = 0;
  total_bytes = 0;
  bytes_read = 0;
  overflow = false;
  overflow_page_id = 0;
};

PayloadStream::PayloadStream(BufferPool *bf, PageID leaf_pid, Offset tuple_offset, Offset tuple_end_offset, size_t total_tuple_size, bool overflow, PageID overflow_page_id) {
  buffer_pool = bf;
  curr_pid = leaf_pid;
  curr_page_offset = tuple_offset + TUPLE_HEADER_SIZE;
  curr_page_end = tuple_end_offset;
  total_bytes = total_tuple_size;
  bytes_read = 0;
  this->overflow = overflow;
  this->overflow_page_id = overflow_page_id;
};

Offset PayloadStream::ReadPage(Byte* page, Byte* buffer, size_t n, Offset start_offset, Offset max_offset) {

  size_t possible_reads = max_offset - start_offset + 1;
  if (n <= possible_reads) {
    memcpy(buffer, page + start_offset, n);
    return start_offset + n;
  } else {
    memcpy(buffer, page + start_offset, possible_reads);
    return start_offset + possible_reads;
  };
};

size_t PayloadStream::NextBytes(Byte* buffer, size_t n) {
  
  size_t total_possible_read_size = total_bytes - bytes_read;
  size_t bytes_to_read = std::min(n, total_possible_read_size);

  if (total_possible_read_size == 0) return 0;

  Result<Byte*> curr_page_fetch = buffer_pool->RequestPage(curr_pid);
  Byte* curr_page = curr_page_fetch.value;

  size_t total_read_this_call = 0;

  while (bytes_to_read > 0) {

    // Returns the offset after the byte it read last.
    Offset result = PayloadStream::ReadPage(curr_page, buffer + total_read_this_call, bytes_to_read, curr_page_offset, curr_page_end - 1);

    size_t bytes_read_from_page = result - curr_page_offset; 
    bytes_read += bytes_read_from_page; 
    bytes_to_read = bytes_to_read - bytes_read_from_page;
    total_read_this_call += bytes_read_from_page;

    if (result == curr_page_end) {
      if (overflow) {
        buffer_pool->ReleasePage(curr_pid, false);
        curr_pid = overflow_page_id;
        curr_page_offset = OVERFLOW_PAGE_HEADER_SIZE;
        curr_page_end = PAGE_SIZE;
        curr_page_fetch = buffer_pool->RequestPage(curr_pid);
        curr_page = curr_page_fetch.value;
        OverflowPageHeader* curr_page_header = reinterpret_cast<OverflowPageHeader*>(curr_page); 
        overflow = curr_page_header->overflow > 0 ? true : false;
        overflow_page_id = curr_page_header->overflow_page;
      } else {
        break;
      };
    } else {
      curr_page_offset = result;
      break;
    }
  };
  
  buffer_pool->ReleasePage(curr_pid, false);
  return total_read_this_call;
};

// Takes in a buffer [data] of size buffer_size and writes it in the leaf page.
WriteStatus LeafPage::WriteChunkLeaf(Byte* page, Byte *buffer, BufferSize buffer_size, Key key, TupleHeader* default_tuple) {

  // We know the minimum space is available because otherwise node would have split.
  uint32_t data_size = std::min((uint32_t)(SLOT_SIZE + TUPLE_HEADER_SIZE + buffer_size), (uint32_t)MAX_LEAF_PAGE_DATA);
  uint32_t available_space = LeafPage::CheckAvailableSpace(page);
  uint32_t total_space = available_space + LeafPage::CheckGarbageBytes(page);
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  
  if (data_size <= total_space) {
    if (data_size > available_space) {
      LeafPage::DefragmentPage(page);
    };

    uint16_t payload_size = data_size - SLOT_SIZE;

    SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
    SlotArrayElement* slot_array_end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE + (SLOT_SIZE * page_header->slot_array_size));

    SlotArrayElement* it = LeafPage::upper_bound(slot_array_start, slot_array_end, page, key);

    if (it == slot_array_start) {
      memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);
      it->offset = page_header->free_space_end_offset - payload_size + 1;
      it->length = payload_size;
      it->is_deleted = 0;
      page_header->slot_array_size++;
    } else {
      if ((it - 1)->is_deleted > 0) {
        it = it - 1;
        it->offset = page_header->free_space_end_offset - payload_size + 1;
        it->length = payload_size;
        it->is_deleted = 0;
      } else {
        memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);
        it->offset = page_header->free_space_end_offset - payload_size + 1;
        it->length = payload_size;
        it->is_deleted = 0;
        page_header->slot_array_size++;
      };
    };

    page_header->free_space_end_offset = it->offset - 1;

    memcpy(page + it->offset + TUPLE_HEADER_SIZE, buffer, payload_size - TUPLE_HEADER_SIZE); 
    TupleHeader* th = reinterpret_cast<TupleHeader*>(page + it->offset);
    if (default_tuple == nullptr) {
      uint32_t size = static_cast<uint32_t>(buffer_size);    
      th->overflow = 0;
      th->overflow_page = 0;
      th->size = size;
    } else {
      *th = *default_tuple;
    };

    return { .written = (uint16_t)(payload_size - TUPLE_HEADER_SIZE), .overflow_info_store_address = page + it->offset };
  } else {

    uint16_t payload_size = total_space - SLOT_SIZE;

    if (available_space < total_space) {
      LeafPage::DefragmentPage(page);
    };

    SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
    SlotArrayElement* slot_array_end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE + (SLOT_SIZE * page_header->slot_array_size));

    SlotArrayElement* it = LeafPage::upper_bound(slot_array_start, slot_array_end, page, key);
    if (it == slot_array_start) {
      memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);
      it->offset = page_header->free_space_end_offset - payload_size + 1;
      it->length = payload_size;
      it->is_deleted = 0;
      page_header->slot_array_size++;
    } else {
      if ((it - 1)->is_deleted > 0) {
        it = it - 1;
        it->offset = page_header->free_space_end_offset - payload_size + 1;
        it->length = payload_size;
        it->is_deleted = 0;
      } else {
        memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);
        it->offset = page_header->free_space_end_offset - payload_size + 1;
        it->length = payload_size;
        it->is_deleted = 0;
        page_header->slot_array_size++;
      };
    };

    page_header->free_space_end_offset = it->offset - 1;

    memcpy(page + it->offset + TUPLE_HEADER_SIZE, buffer, (uint64_t)(payload_size - TUPLE_HEADER_SIZE)); 
    TupleHeader* th = reinterpret_cast<TupleHeader*>(page + it->offset);
    if (default_tuple == nullptr) {
      uint32_t size = static_cast<uint32_t>(buffer_size);    
      th->overflow = 0;
      th->overflow_page = 0;
      th->size = size;
    } else {
      *th = *default_tuple;
    };

    return { .written = (uint16_t)(payload_size - TUPLE_HEADER_SIZE), .overflow_info_store_address = page + it->offset };
  };
};

WriteStatus LeafPage::WriteChunkOverflow(Byte* page, Byte *buffer, BufferSize buffer_size) {
  if (buffer_size + OVERFLOW_PAGE_HEADER_SIZE > PAGE_SIZE) {
    uint16_t written_size = PAGE_SIZE - OVERFLOW_PAGE_HEADER_SIZE;
    memcpy(page + OVERFLOW_PAGE_HEADER_SIZE, buffer, written_size);
    return { .written = written_size, .overflow_info_store_address = page + OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET }; 
  } else {
    memcpy(page + OVERFLOW_PAGE_HEADER_SIZE, buffer, buffer_size);
    OverflowInfo* info = reinterpret_cast<OverflowInfo*>(page + OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET);
    info->overflow = 0;
    info->overflow_page = 0;
    return { .written = static_cast<uint16_t>(buffer_size), .overflow_info_store_address = nullptr };
  };
};

BorrowQuery LeafPage::CanLendFromRight(Byte* page, uint16_t needed) {
  
  uint16_t usedspace = PAGE_SIZE - LeafPage::CheckAvailableSpace(page) - LeafPage::CheckGarbageBytes(page) - LEAF_PAGE_HEADER_SIZE;

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  SlotArrayElement* curr = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE) + page_header->slot_array_size - 1;
  SlotArrayElement* rev_end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE) - 1;
  uint16_t count_space = 0;
  uint16_t brw_count = 0;

  while (count_space < needed) {

    if (curr == rev_end) {
      break;
    };

    if (curr->is_deleted > 0) {
      curr--;
      continue;
    };

    uint16_t curr_size = curr->length;
    usedspace -= curr_size + SLOT_SIZE;
    
    if (usedspace <= LEAF_UNDERFLOW_THRESHOLD) {
      return { .can_borrow = false, .borrow_amount = 0 };
    };

    count_space += curr_size + SLOT_SIZE;
    brw_count++;
    curr--;
    
    if (count_space >= needed) {
      return { .can_borrow = true, .borrow_amount = brw_count };
    };
  };

  return { .can_borrow = false, .borrow_amount = 0 };
};

BorrowQuery LeafPage::CanLendFromLeft(Byte* page, uint16_t needed) {
  
  uint16_t usedspace = PAGE_SIZE - LeafPage::CheckAvailableSpace(page) - LeafPage::CheckGarbageBytes(page) - LEAF_PAGE_HEADER_SIZE;

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  SlotArrayElement* curr = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
  SlotArrayElement* end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE) + page_header->slot_array_size;
  uint16_t count_space = 0;
  uint16_t brw_count = 0;

  while (count_space < needed) {

    if (curr == end) {
      break;
    };

    if (curr->is_deleted > 0) {
      curr++;
      continue;
    };

    uint16_t curr_size = curr->length;
    usedspace -= curr_size + SLOT_SIZE;
    
    if (usedspace <= LEAF_UNDERFLOW_THRESHOLD) {
      return { .can_borrow = false, .borrow_amount = 0 };
    };

    count_space += curr_size + SLOT_SIZE;
    brw_count++;
    curr++;
    
    if (count_space >= needed) {
      return { .can_borrow = true, .borrow_amount = brw_count };
    };
  };

  return { .can_borrow = false, .borrow_amount = 0 };
};

Key LeafPage::HandleLeftBorrow(Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report) {

  LeafPageHeader* lender_header = reinterpret_cast<LeafPageHeader*>(lender_page);
  SlotArrayElement* lender_curr = reinterpret_cast<SlotArrayElement*>(lender_page + LEAF_PAGE_HEADER_SIZE) + lender_header->slot_array_size - 1;
  ssize_t borrow_amount = borrow_report.borrow_amount;

  while (borrow_amount) {

    if (lender_curr->is_deleted > 0) {
      lender_curr--;
      continue;
    };

    Key key = LeafPage::GetKeyFromSlotElement(lender_page, lender_curr);
    
    // insert tuple from the sibling to this page and defragment if needed.
    LeafPage::WriteChunkLeaf(borrower_page,
        lender_page + lender_curr->offset + TUPLE_HEADER_SIZE,
        lender_curr->length - TUPLE_HEADER_SIZE,
        key, reinterpret_cast<TupleHeader*>(lender_page + lender_curr->offset));

    lender_header->garbage_bytes += lender_curr->length;
    lender_curr->is_deleted = 1;
    lender_curr--;
    borrow_amount--;
  };

  Key boundary_key = LeafPage::GetPageFirstKey(borrower_page);

  return boundary_key;
};


Key LeafPage::HandleRightBorrow(Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report) {

  LeafPageHeader* lender_header = reinterpret_cast<LeafPageHeader*>(lender_page);
  SlotArrayElement* lender_curr = reinterpret_cast<SlotArrayElement*>(lender_page + LEAF_PAGE_HEADER_SIZE);
  ssize_t borrow_amount = borrow_report.borrow_amount;

  while (borrow_amount) {

    if (lender_curr->is_deleted > 0) {
      lender_curr++;
      continue;
    };

    Key key = LeafPage::GetKeyFromSlotElement(lender_page, lender_curr);    
    // insert tuple from the sibling to this page and defragment if needed.
    LeafPage::WriteChunkLeaf(borrower_page,
        lender_page + lender_curr->offset + TUPLE_HEADER_SIZE,
        lender_curr->length - TUPLE_HEADER_SIZE,
        key, reinterpret_cast<TupleHeader*>(lender_page + lender_curr->offset));

    lender_header->garbage_bytes += lender_curr->length;
    lender_curr->is_deleted = 1;
    lender_curr++;
    borrow_amount--;
  };

  Key boundary_key = LeafPage::GetPageFirstKey(lender_page);
  return boundary_key;
};

void LeafPage::MergePages(Byte* to_page, Byte* from_page) {

  LeafPageHeader* to_header = reinterpret_cast<LeafPageHeader*>(to_page);
  LeafPageHeader* from_header = reinterpret_cast<LeafPageHeader*>(from_page);

  to_header->right_pid = from_header->right_pid;

  // std::cout << to_header->right_pid << std::endl;
  // std::cout << to_header->slot_array_size<< std::endl;
  // Iterate over the from_page slot and tuple, then insert each of them into the to_page.
  // First insert the slot properly.
  // Then insert the data using the write_chunk function.
  // The free_space offset index will be set itself using the write_chunk function.
  // Better separate the insert slot logic cause it will be used in the btree insert too.

  SlotArrayElement* from_slot_array = reinterpret_cast<SlotArrayElement*>(from_page + LEAF_PAGE_HEADER_SIZE);
  // WriteChunkLeaf(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key, TupleHeader* default_tuple = nullptr)

  for (int i=0; i<from_header->slot_array_size; i++) {
  
    SlotArrayElement* element = from_slot_array + i;
    if (element->is_deleted > 0) continue;
    
    Key key = LeafPage::GetKeyFromSlotElement(from_page, element); 

    uint16_t freespace = LeafPage::CheckGarbageBytes(to_page) + CheckAvailableSpace(to_page);

    if (freespace < element->length + SLOT_SIZE) {
      throw std::runtime_error("[FATAL] Trying to merge pages but too much data to put into a page.\n");
      std::exit(1);
    };
    
    LeafPage::WriteChunkLeaf(to_page,
        from_page + element->offset + TUPLE_HEADER_SIZE,
        element->length - TUPLE_HEADER_SIZE,
        key, reinterpret_cast<TupleHeader*>(from_page + element->offset));
  };

  // std::cout << to_header->right_pid << std::endl;
  // std::cout << to_header->slot_array_size<< std::endl;
  return;
};

// MakePage(Byte* page, SlotArrayElement* slot_array_start, uint16_t slot_array_size, Byte* buffer, PageID pid, PageID left_pid, PageID right_pid);
void LeafPage::DefragmentPage(Byte* page) {

  Byte buffer[PAGE_SIZE];
  memcpy(buffer, page, PAGE_SIZE);

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(buffer);
  SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(buffer + LEAF_PAGE_HEADER_SIZE);

  LeafPage::MakePage(page, slot_array_start, page_header->slot_array_size, buffer, page_header->page_id, page_header->left_pid, page_header->right_pid);
};


Key LeafPage::GetPageFirstKey(Byte* page) {

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* slot_array = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
  for (int i=0; i<page_header->slot_array_size; i++) {
    SlotArrayElement* element = slot_array + i;
    if (element->is_deleted == 0) {
      return LeafPage::GetKeyFromSlotElement(page, element);
    };
  };
  return (Key)(0);  
};

// lower_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x);
bool LeafPage::DeleteTuple(Byte* page, Key key) {
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
  SlotArrayElement* end = start + page_header->slot_array_size;
  SlotArrayElement* element = lower_bound(start, end, page, key);

  if (element == end) {
    return false;
  };

  Key found_key = LeafPage::GetKeyFromSlotElement(page, element);
  
  if (found_key != key) {
    return false;
  };

  if (element->is_deleted > 0) {
    return false;
  };

  page_header->garbage_bytes += element->length;
  element->is_deleted = 1;
  
  return true;
};

