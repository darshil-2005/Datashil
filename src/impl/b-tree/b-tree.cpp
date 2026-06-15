
// b-tree only talks to buffer pool so buffer pool need to allocate fresh page
// and also need to deal with freshly freed page.

BTree::BTree(BufferPoll &bf) { this->buffer_pool = &bf; };

bool BTree::InsertTuple(const Byte *buffer, BufferSize buffer_size, Key key) {

  NewPage to_write_page;
  SplitReport report = BTree::FindPageToWrite(root_page_id, key, buffer_size, &to_write_page);

  if (report.was_split) {
    Record<NewPage> new_root_result = buffer_pool->AllocateNewPage();
    // handle error
    PageID new_root_id = new_root_result.value->pid;
    Key keys_ptr[1] = { report.boundary_key };
    PageID children_ptr[2] = { root_page_id, report.new_page_id };
    InternalPage::MakePage(new_root_result.value->ptr, keys_ptr, children_ptr, 1, new_root_id);
    root_page_id = new_root_id;
  };

  // WriteStatus returns the pointer to the byte that starts at overflow flag of 1 byte and followed by 2 bytes of overflow_page_id
  WriteStatus write_status = BTree::WriteChunkLeaf(to_write_page.ptr, buffer, buffer_size, key);
  
  if (write_status.written == buffer_size) {
    buffer_pool->ReleasePage(to_write_page.pid, true);
    return true;
  };

  buffer = buffer + write_status.written;
  buffer_size = buffer_size - write_status.written;

  while (buffer_size > 0) {
    Result<NewPage> new_page_response = buffer_pool->AllocateNewPage();
    // handle error
    NewPage new_page = new_page_response.value;
    OverflowPageHeader* page_header = reinterpret_cast<OverflowPageHeader*>(new_page.ptr);
    page_header->page_type = PageType::OverflowPage;
    page_header->page_id = new_page.pid;
    OverflowInfo* overflow_info = reinterpret_cast<OverflowInfo*>(write_status.overflow_info_store_address);
    overflow_info->overflow = 1;
    overflow_info->overflow_page = new_page.pid;

    buffer_pool->ReleasePage(new_page.pid, true);

    write_status = BTree::WriteChunkOverflow(new_page.ptr, buffer, buffer_size);

    buffer = buffer + write_status.written;
    buffer_size = buffer_size - write_status.written;

    if (buffer_size == 0) {
      buffer_pool->ReleasePage(new_page.pid, true);
    };
  };

  return true;
};

SplitReport BTree::FindPageToWrite(PageID pid, Key key, BufferSize buffer_size, NewPage *to_write_page) {

  Result<Byte *> page_result = buffer_pool->RequestPage(pid);
  // handle errors here

  Byte *page = page_result.value;
  PageHeader *current_page_header = reinterpret_cast<PageHeader *>(page);

  if (current_page_header->page_type == PageType::LeafPage) {

    uint16_t available_space = LeafPage::CheckAvailableSpace(page);
    uint16_t min_entry = SLOT_SIZE + TUPLE_HEADER_SIZE + min(MIN_LEAF_PAGE_DATA, buffer_size);

    if (min_entry <= available_space) {
      *to_write_page = {.ptr = page, .pid = current_page_header->page_id};
      return {
          .was_split = 0,
          .new_page_id = 0,
          .boundary_key = 0,
      };
    };

    Result<NewPage> new_page_result = buffer_pool->AllocateNewPage();
    // handle errors here

    NewPage new_page = new_page_result.value;

    // The returns the lowest key of the new page to us as boundary key.
    // new_page gets the bigger half of the entries
    uint16_t boundary_key = LeafPage::HandleSplit(page, new_page.ptr, new_page.pid);

    if (key >= boundary_key) {
      *to_write_page = new_page;
      buffer_pool->ReleasePage(current_page_header->page_id, true);
    } else {
      *to_write_page = {.ptr = page, .pid = current_page_header->page_id};
      buffer_pool->ReleasePage(new_page.pid, true);
    };

    return {.boundary_key = boundary_key,
            .was_split = 1,
            .new_page_id = new_page.pid};

  } else if (current_page_header->page_type == PageType::InternalPage) {
    Result<PageID> child_page = InternalPage::GetChildPageID(page, key);

    // handle error from above if any

    PageID child_page_id = child_page.value;
    SplitReport report =
        FindPageToWrite(child_page_id, key, buffer_size, to_write_page);

    // handle errors if any here

    if (report.was_split != 0) {

      // check if there is sufficient space in the current node
      InternalPageHeader *internal_page_header =
          reinterpret_cast<InternalPageHeader *>(page);
      // hard coded for now will change if there is variable sized keys.
      Bool slot_available = InternalPage::CheckSlotAvailable(page, KEY_SIZE);

      if (slot_available > 0) {
        Bool result = InternalPage::InsertKeyValue(page, report.boundary_key,
                                                   report.new_page_id);
        if (result == 0) {
          // handle errors
        };
        buffer_pool->ReleasePage(current_page_header->page_id, true);
        return {.was_split = 0, .new_page_id = 0, .boundary_key = 0};

      } else {
        Result<NewPage> new_page_result = buffer_pool->AllocateNewPage();
        // handle errors here
        NewPage new_page = new_page_result.value;

        uint16_t boundary_key = InternalPage::HandleSplit(
            page, new_page, report.boundary_key, report.new_page_id);
        buffer_pool->ReleasePage(pid, true);
        buffer_pool->ReleasePage(new_page.pid, true);

        return {.boundary_key = boundary_key,
                .was_split = 1,
                .new_page_id = new_page.pid};
      };
    } else {
      buffer_pool->ReleasePage(pid, false);
      return {.boundary_key = 0, .was_split = 0, .new_page_id = 0};
    };
  };
};

WriteStatus BTree::WriteChunkLeaf(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key) {
    
  // We know the minimum space is available because otherwise node would have split.
  uint16_t data_size = SLOT_SIZE + TUPLE_HEADER_SIZE + min(buffer_size, MAX_LEAF_PAGE_DATA);
  uint16_t available_space = LeafPage::CheckAvailableSpace(page);
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  if (data_size <= available_space) {

    uint16_t payload_size = TUPLE_HEADER_SIZE + min(buffer_size, MAX_LEAF_PAGE_DATA);

    SlotArrayElement* slot_array_start = page + LEAF_PAGE_HEADER_SIZE;
    SlotArrayElement* slot_array_end = page + (SLOT_SIZE * page_header->slot_array_size);

    SlotArrayElement* it = upper_bound(slot_array_start, slot_array_end, page, key);
    if (it != slot_array_end) memmove(it+SLOT_SIZE, it, (slot_array_end - it) * SLOT_SIZE);

    it->offset = free_space_end_offset - payload_size + 1;
    it->length = payload_size;

    memcpy(page + it->offset, buffer, payload_size); 
    return { .written = payload_size, .overflow_info_store_address = nullptr };

  } else {

    uint16_t payload_size = available_space - SLOT_SIZE;

    SlotArrayElement* slot_array_start = page + LEAF_PAGE_HEADER_SIZE;
    SlotArrayElement* slot_array_end = page + (SLOT_SIZE * page_header->slot_array_size);

    SlotArrayElement* it = upper_bound(slot_array_start, slot_array_end, page, key);
    if (it != slot_array_end) memmove(it+SLOT_SIZE, it, (slot_array_end - it) * SLOT_SIZE);

    it->offset = free_space_end_offset - payload_size + 1;
    it->length = payload_size;

    memcpy(page + it->offset, buffer, payload_size); 
    return { .written = payload_size, .overflow_info_store_address = page + it->offset };
  };
};

WriteStatus BTree::WriteChunkOverflow(Byte* page, const Byte *buffer, BufferSize buffer_size) {

  if (buffer_size + OVERFLOW_PAGE_HEADER_SIZE > PAGE_SIZE) {
    uint16_t written_size = PAGE_SIZE - OVERFLOW_PAGE_HEADER_SIZE;
    memcpy(page + OVERFLOW_PAGE_HEADER_SIZE, buffer, written_size);
    return { .written = written_size, .overflow_info_store_address = page + OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET } 

  } else {
    memcpy(page, buffer, buffer_size);
    return { .written = buffer_size, .overflow_info_store_address = nullptr };
  };
};

SearchResult BTree::Search(PageID pid, Key key) {

  Result<Byte*> request_page_response = buffer_pool->RequestPage(pid);
  // Handle errors

  Byte* page = request_page_response->value;
  PageHeader* general_page_header = reinterpret_cast<PageHeader*>(page);

  if (general_page_header->page_type == PageType::InternalPage) {
    
    PageID child_pid = InternalPage::GetChildPageID(page, key);
    buffer_pool->ReleasePage(pid, false);
    return BTree::Search(pid, key);

  } else if (general_page_header->page_type == PageType::LeafPage) {

    SearchResult result = LeafPage::Search(page, key);
    buffer_pool->ReleasePage(pid, false);
    return result;

  };
};

