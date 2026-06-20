#include "../../include/page/leaf_page.h"

// need to handle both leaf and overflow page
uint16_t LeafPage::CheckAvailableSpace(Byte* page) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  Byte* free_space_start = page;
  Byte* free_space_end = page + page_header->free_space_end_offset;
  free_space_start = free_space_start + LEAF_PAGE_HEADER_SIZE;
  free_space_start = free_space_start + (page_header->slot_array_size * SLOT_SIZE);

  uint16_t free_bytes = free_space_end - free_space_start + 1;
  return free_bytes;
};

SlotArrayElement* LeafPage::upper_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x) {

  int16_t n = end - start;
  int16_t l = 0;
  int16_t r = n - 1;
  int16_t ans = n;

  while (l <= r) {
    uint16_t mid = l + (r - l) / 2;
    SlotArrayElement* element = start + mid;
    Byte* tuple = page + element->offset;
    Key* key = reinterpret_cast<Key*>(tuple + TUPLE_HEADER_SIZE); 
    if (*key <= x) {
      l = mid + 1;
    } else {
      ans = mid;
      r = mid - 1;
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
  SlotArrayElement* slot_array_new_page_start = reinterpret_cast<SlotArrayElement*>(buffer + LEAF_PAGE_HEADER_SIZE);

  for (int i=0; i < page_header->slot_array_size; i++) {
    SlotArrayElement* current = slot_array_start + i;
    consumed_space = consumed_space + current->length;
    if (consumed_space > threshold) {
      slot_array_new_page_start = current + 1;      
      break;
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

  page_header->slot_array_size = slot_array_size;
  page_header->page_type = PageType::LeafPage;
  page_header->page_id = pid;
  page_header->left_pid = left_pid;
  page_header->right_pid = right_pid;

  if (slot_array_size == 0) {
    page_header->free_space_end_offset = PAGE_SIZE - 1;
    return true;
  };

  SlotArrayElement* slot_array = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);

  for (int i=0; i<slot_array_size; i++) {
    PageOffset offset = slot_array_start[i].offset;
    TupleLength length = slot_array_start[i].length;

    memcpy(page + free_space_end_offset - length + 1, buffer + offset, length); 
    slot_array[i].offset = free_space_end_offset - length + 1;
    slot_array[i].length = length;
    free_space_end_offset = slot_array[i].offset;
  };

  page_header->free_space_end_offset = free_space_end_offset;
  
  return true;
};

SlotArrayElement* LeafPage::lower_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x) {

  int16_t n = end - start;
  int16_t l = 0;
  int16_t r = n - 1;
  int16_t ans = n;

  while (l <= r) {
    uint16_t mid = l + (r - l) / 2;
    SlotArrayElement* element = start + mid;
    Byte* tuple = page + element->offset;
    Key* key = reinterpret_cast<Key*>(tuple + TUPLE_HEADER_SIZE); 
    if (*key < x) {
      l = mid + 1;
    } else {
      ans = mid;
      r = mid - 1;
    };
  };

  return start + ans;
};

Key LeafPage::GetKeyFromSlotElement(Byte* page, SlotArrayElement* element) {
  uint16_t key;
  Byte* key_address = page + element->offset + TUPLE_HEADER_SIZE;
  memcpy(&key, key_address, sizeof(uint16_t));
  return key;
};

uint32_t LeafPage::GetTupleSizeFromSlotElement(Byte* page, SlotArrayElement* element) {
  TupleHeader* tuple_header = reinterpret_cast<TupleHeader*>(page + element->offset);
  return tuple_header->size;
};

SearchResult LeafPage::Search(Byte* page, Key key) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
  SlotArrayElement* slot_array_end = reinterpret_cast<SlotArrayElement*>(slot_array_start + page_header->slot_array_size);
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
WriteStatus LeafPage::WriteChunkLeaf(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key) {

  // We know the minimum space is available because otherwise node would have split.
  uint16_t data_size = std::min(SLOT_SIZE + TUPLE_HEADER_SIZE + buffer_size, MAX_LEAF_PAGE_DATA);
  uint16_t available_spac e = LeafPage::CheckAvailableSpace(page);
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  
  if (data_size <= available_space) {

    uint16_t payload_size = data_size - SLOT_SIZE;

    SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
    SlotArrayElement* slot_array_end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE + (SLOT_SIZE * page_header->slot_array_size));

    SlotArrayElement* it = LeafPage::upper_bound(slot_array_start, slot_array_end, page, key);

    if (it != slot_array_end) memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);
    
    it->offset = page_header->free_space_end_offset - payload_size + 1;
    it->length = payload_size;

    page_header->free_space_end_offset = it->offset - 1;
    page_header->slot_array_size++;

    uint32_t size = static_cast<uint32_t>(buffer_size);    
    memcpy(page + it->offset + sizeof(OverflowInfo), &size, sizeof(size));
    memcpy(page + it->offset + TUPLE_HEADER_SIZE, buffer, payload_size - TUPLE_HEADER_SIZE); 
    TupleHeader* th = reinterpret_cast<TupleHeader*>(page + it->offset);

    return { .written = (uint16_t)(payload_size - TUPLE_HEADER_SIZE), .overflow_info_store_address = page + it->offset };

  } else {

    uint16_t payload_size = available_space - SLOT_SIZE;

    SlotArrayElement* slot_array_start = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);
    SlotArrayElement* slot_array_end = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE + (SLOT_SIZE * page_header->slot_array_size));

    SlotArrayElement* it = LeafPage::upper_bound(slot_array_start, slot_array_end, page, key);
    if (it != slot_array_end) memmove(it+1, it, (slot_array_end - it) * SLOT_SIZE);

    it->offset = page_header->free_space_end_offset - payload_size + 1;
    it->length = payload_size;

    page_header->free_space_end_offset = it->offset - 1;
    page_header->slot_array_size++;

    uint32_t size = static_cast<uint32_t>(buffer_size);    
    memcpy(page + it->offset + sizeof(OverflowInfo), &size, sizeof(size));
    memcpy(page + it->offset + TUPLE_HEADER_SIZE, buffer, (uint16_t)(payload_size - TUPLE_HEADER_SIZE)); 
    return { .written = (uint16_t)(payload_size - TUPLE_HEADER_SIZE), .overflow_info_store_address = page + it->offset };
  };
};

WriteStatus LeafPage::WriteChunkOverflow(Byte* page, const Byte *buffer, BufferSize buffer_size) {

  if (buffer_size + OVERFLOW_PAGE_HEADER_SIZE > PAGE_SIZE) {
    uint16_t written_size = PAGE_SIZE - OVERFLOW_PAGE_HEADER_SIZE;
    memcpy(page + OVERFLOW_PAGE_HEADER_SIZE, buffer, written_size);
    return { .written = written_size, .overflow_info_store_address = page + OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET }; 

  } else {
    memcpy(page + OVERFLOW_PAGE_HEADER_SIZE, buffer, buffer_size);
    return { .written = buffer_size, .overflow_info_store_address = nullptr };
  };
};

BorrowQuery LeafPage::CanLendFromRight(PageID pid, uint16_t needed) {
  
  Request<Byte*> page_request = buffer_pool->RequestPage(pid);

  // handle errors

  Byte* page = page_request.value;
  uint16_t usedspace = LeafPage::GetCurrentUsedSpace(page);

  LeafPageHeader* page_header = reinterpret_cast<LeafPageheader*>(page);

  SlotArrayElement* curr = LeafPage::GetLastSlotArrayElement(page);
  SlotArrayElement* rev_end = LeafPage::GetSlotArrayReverseEnd(page);
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
    usedspace -= curr_size;
    
    if (usedspace <= LEAF_PAGE_UNDERFLOW_THRESHOLD) {
      buffer_pool->ReleasePage(pid, false);
      return { .can_borrow = false, .borrow_amount = 0, .lender = pid };
    };

    count_space += curr_size;
    brw_count++;
    curr--;
    
    if (count_space >= needed) {
      buffer_pool->ReleasePage(pid, false);
      return { .can_borrow = true, .borrow_amount = brw_count, .lender = pid };
    };
  };

  buffer_pool->ReleasePage(pid, false);
  return { .can_borrow = false, .borrow_amount = 0, .lender = pid };
};

BorrowQuery LeafPage::CanLendFromLeft(PageID pid, uint16_t needed) {
  
  Request<Byte*> page_request = buffer_pool->RequestPage(pid);

  // handle errors

  Byte* page = page_request.value;
  uint16_t usedspace = LeafPage::GetCurrentUsedSpace(page);

  LeafPageHeader* page_header = reinterpret_cast<LeafPageheader*>(page);

  SlotArrayElement* curr = LeafPage::GetFirstSlotArrayElement(page);
  SlotArrayElement* end = LeafPage::GetSlotArrayEnd(page);
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
    usedspace -= curr_size;
    
    if (usedspace <= LEAF_PAGE_UNDERFLOW_THRESHOLD) {
      buffer_pool->ReleasePage(pid, false);
      return { .can_borrow = false, .borrow_amount = 0, .lender = pid };
    };

    count_space += curr_size;
    brw_count++;
    curr++;
    
    if (count_space >= needed) {
      buffer_pool->ReleasePage(pid, false);
      return { .can_borrow = true, .borrow_amount = brw_count, .lender = pid };
    };
  };

  buffer_pool->ReleasePage(pid, false);
  return { .can_borrow = false, .borrow_amount = 0, .lender = pid };
};

Key LeafPage::HandleLeftBorrow(PageID pid, BorrowQuery borrow_report) {

  Result<Byte*> left_sibling_request = buffer_pool->RequestPage(borrow_report.lender);
  // handle errors
  Byte* left_sib_page = left_sibling_request.value;

  Result<Byte*> page_request = buffer_pool->RequestPage(pid);
  // handle errors
  Byte* page = page_request.value;

  LeafPageHeader* sib_header = reinterpret_cast<LeafPageHeader*>(left_sib_page);

  SlotArrayElement* sib_curr = LeafPage::GetLastSlotArrayElement(left_sib_page);

  while (borrow_amount) {

    if (sib_curr->is_deleted > 0) {
      sib_curr--;
      continue;
    };
    
    // insert tuple from the sibling to this page and defragment if needed.

    sib_header->garbage_bytes += sib_curr->length;
    sib_curr->is_deleted = 1;
    sib_curr--;
    borrow_amount--;
  };

  Key boundary_key = LeafPage::GetPageFirstKey(page);

  buffer_pool->ReleasePage(borrow_report->lender, true);
  buffer_pool->ReleasePage(pid, true);

  return boundary_key;
};


Key LeafPage::HandleRightBorrow(PageID pid, BorrowQuery borrow_report) {

  Result<Byte*> right_sibling_request = buffer_pool->RequestPage(borrow_report.lender);
  // handle errors
  Byte* right_sib_page = left_sibling_request.value;

  Result<Byte*> page_request = buffer_pool->RequestPage(pid);
  // handle errors
  Byte* page = page_request.value;

  LeafPageHeader* sib_header = reinterpret_cast<LeafPageHeader*>(right_sib_page);

  SlotArrayElement* sib_curr = LeafPage::GetFirstSlotArrayElement(right_sib_page);

  while (borrow_amount) {

    if (sib_curr->is_deleted > 0) {
      sib_curr++;
      continue;
    };
    
    // insert tuple from the sibling to this page and defragment if needed.

    sib_header->garbage_bytes += sib_curr->length;
    sib_curr->is_deleted = 1;
    sib_curr++;
    borrow_amount--;
  };

  Key boundary_key = LeafPage::GetPageFirstKey(page);

  buffer_pool->ReleasePage(borrow_report->lender, true);
  buffer_pool->ReleasePage(pid, true);

  return boundary_key;
};

void MergePages(PageID to_pid, PageID from_pid) {
  
  Result<Byte*> to_page_request = buffer_pool->RequestPage(to_pid);
  // handle errors
  Byte* to_page = to_page_request.value;

  Result<Byte*> from_page_request = buffer_pool->RequestPage(from_pid);
  // handle errors
  Byte* from_page = from_page_request.value;

  
  // Iterate over the from_page tuples and insert them into the to_page using the insert function.


  




};

