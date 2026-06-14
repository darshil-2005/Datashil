
// b-tree only talks to buffer pool so buffer pool need to allocate fresh page
// and also need to deal with freshly freed page.

BTree::BTree(BufferPoll &bf) { this->buffer_pool = &bf; };

Result<SplitReport> BTree::InsertTuple(PageID pid, const Byte *buffer,
                                       BufferSize buffer_size,
                                       const uint16_t key, RecordID *result) {

  // handling the case where we are at an internal page.
  // This function traveses the tree and reaches the leaf page where we need to
  // insert; then tries to insert. If succeeds fine if not try to deal with that
  // issue by splitting the page. and then recursively deals with the issues.
  // 
  //
  // buffer only has the data of the tuple rest has to be added by us
  // data of the tuple always starts with a 2 byte key.

  Result<Byte *> page = buffer_pool->RequestPage(pid);

  if (page.err != ErrType::None) {
    // handle errors here
  };

  PageHeader *current_page_header = reinterpret_cast<PageHeader *>(page);

  if (current_page_header->page_type == PageType::LeafPage) {

    uint16_t available_space = LeafPage::CheckAvailableSpace(page);
    uint16_t tuple_space = min(TUPLE_SIZE_LIMIT, buffer_size + TUPLE_HEADER_SIZE);

    if (tuple_space + SLOT_SIZE <= available_space) {
      RecordID record_location =
          LeafPage::InsertTuple(page, buffer, buffer_size);
      *result = record_location;
      buffer_pool->ReleasePage(pid, true);
      return {
          .value = {.was_split = 0, .new_page_id = 0, .boundary_key = 0},
      };
    };

    Result<NewPage> new_page = buffer_pool->AllocateNewPage();
    // handle errors here

    uint16_t boundary_key = LeafPage::HandleSplit(page, new_page, buffer, buffer_size);
    buffer_pool->ReleasePage(pid, true);
    buffer_pool->ReleasePage(new_page.pid, true);
    return {.value = {.boundary_key = boundary_key,
                      .was_split = 1,
                      .new_page_id = new_page.pid},
            .err = ErrType::None};

  } else if (current_page_header->page_type == PageType::InternalPage) {
    Result<PageID> child_page = InternalPage::GetChildPageID(page, key);

    // handle error from above if any

    PageID child_page_id = child_page.value;

    Result<SplitReport> recursion_result =
        InsertTuple(child_page_id, buffer, buffer_size, key, result);

    // handle errors if any here

    SplitReport report = recursion_result.value;

    if (report.was_split != 0) {

      // check if there is sufficient space in the current node
      InternalPageHeader *internal_page_header =
          reinterpret_cast<InternalPageHeader *>(page);
      // hard coded for now will change if there is variable sized keys.
      Bool slot_avalaible = InternalPage::CheckSlotAvailable(page, KEY_SIZE);

      if (slot_available > 0) {
        Bool result =
            InternalPage::InsertKeyValue(page, report.boundary_key, report.new_pid);
        if (result == 0) {
          // handle errors
        };
        buffer_pool->ReleasePage(pid, true);
        return {.value = {.was_split = 0, .new_page_id = 0, .boundary_key = 0},
                .err = ErrType::None};
      } else {
        Result<NewPage> new_page = buffer_pool->AllocateNewPage();
        // handle errors here

        uint16_t boundary_key = InternalPage::HandleSplit(page, new_page, report.boundary_key, report.new_pid);
        buffer_pool->ReleasePage(pid, true);
        buffer_pool->ReleasePage(new_page.pid, true);

        return {.value = {.boundary_key = boundary_key,
                          .was_split = 1,
                          .new_page_id = new_page.pid},
                .err = ErrType::None};
      };
    } else {
        buffer_pool->ReleasePage(pid, true);
        return {.value = {.boundary_key = 0,
                          .was_split = 0,
                          .new_page_id = 0},
                .err = ErrType::None};
    };

  } else {
    // handle errors here somehow the node is neither.
  }
};
