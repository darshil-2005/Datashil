#include "../../include/page/internal_page.h"

Bool InternalPage::CheckSlotAvailable(Byte* page, uint16_t key_size) {
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  if (page_header->num_keys < NUM_KEY_SLOTS) {
    return 1;
  };
  return 0;
};

Key* InternalPage::GetKeysStartPointer(Byte* page){
  return reinterpret_cast<Key*>(page + INTERNAL_PAGE_HEADER_SIZE);
};

PageID* InternalPage::GetChildrenStartPointer(Byte* page) {
  return InternalPage::GetKeysStartPointer(page) + NUM_KEY_SLOTS;
};

PageID InternalPage::GetValueAtIndex(Byte* page, OffsetIndex index) {
  PageID* value_start = InternalPage::GetChildrenStartPointer(page);
  PageID res = *(value_start + index);
  return res;
};

PageID InternalPage::GetChildPageID(Byte* page, Key key) {
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  Key* key_start = GetKeysStartPointer(page);
  Key* key_end = key_start + page_header->num_keys;

  Key* it = std::upper_bound(key_start, key_end, key);
  OffsetIndex index_child_ptr = it - key_start;

  return GetValueAtIndex(page, index_child_ptr);
};

// ALERT: This will overwrite the child pointers if you enter more keys than capacity.
Bool InternalPage::InsertKeyValue(Byte* page, Key boundary_key, PageID new_pid) {
    
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  Key* key_start = InternalPage::GetKeysStartPointer(page);
  Key* key_end = key_start + page_header->num_keys;
  Key* it = std::upper_bound(key_start, key_end, boundary_key);
  OffsetIndex index_child_ptr = it - key_start;

  memmove(it+1, it, (key_end - it) * KEY_SIZE);

  *it = boundary_key;
  OffsetIndex insertion_index = index_child_ptr + 1;

  PageID* value_start = InternalPage::GetChildrenStartPointer(page);
  PageID* value_end = value_start + page_header->num_keys + 1;
  PageID* insertion_it = value_start + insertion_index;

  if (insertion_it != value_end) {
    memmove(insertion_it+1, insertion_it, (value_end - insertion_it) * sizeof(PageID));
  };
  page_header->num_keys++;

  *insertion_it = new_pid;
  return 1;
};

uint16_t InternalPage::HandleSplit(Byte* old_page, Byte* new_page, Key key_to_insert, PageID page_id_to_insert) {

  Key temp_keys[NUM_KEY_SLOTS + 1];
  PageID temp_ptrs[NUM_CHILD_PAGEID_SLOTS + 1];
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(old_page);
  Key* key_start = InternalPage::GetKeysStartPointer(old_page);
  Key* ptr_start = InternalPage::GetChildrenStartPointer(old_page);

  memcpy(temp_keys, key_start, page_header->num_keys * KEY_SIZE);
  memcpy(temp_ptrs, ptr_start, (page_header->num_keys + 1) * CHILD_PTR_SIZE);

  Key* it = std::upper_bound(temp_keys, temp_keys + page_header->num_keys, key_to_insert);
  
  if (it != temp_keys + page_header->num_keys) {
    memmove(it + 1, it, (temp_keys + page_header->num_keys - it) * KEY_SIZE);
  };

  *it = key_to_insert;
  OffsetIndex index_child_ptr = it - temp_keys;
  OffsetIndex insertion_index = index_child_ptr + 1;

  PageID* insertion_it = temp_ptrs + insertion_index;

  if (insertion_it != temp_ptrs + page_header->num_keys + 1) {
    memmove(insertion_it+1, insertion_it, ((temp_ptrs + page_header->num_keys + 1) - insertion_it) * sizeof(PageID));
  };

  *insertion_it = page_id_to_insert;

  uint16_t new_keys_length = page_header->num_keys + 1; 

  Key* boundary_key = temp_keys + (new_keys_length / 2);
  InternalPage::MakePage(old_page, temp_keys, temp_ptrs, (new_keys_length / 2), page_header->page_id);
  InternalPage::MakePage(new_page, boundary_key + 1, temp_ptrs + (new_keys_length / 2) + 1, ((new_keys_length - 1) / 2), page_id_to_insert);

  return *boundary_key;  
};

// ALERT: This might overwrite the keys into the space for child ptr and write child_ptr over the boundary of the page.
Bool InternalPage::MakePage(Byte* page, Key* keys_ptr, PageID* children_ptr, uint16_t keys_to_take, PageID pid) {

  Byte* curr = page;
  *curr = static_cast<Byte>(PageType::InternalPage);

  curr = curr + sizeof(PageType::InternalPage);
  // correction
  memcpy(curr, &pid, sizeof(pid));

  curr = curr + sizeof(pid);
  memcpy(curr, &keys_to_take, sizeof(keys_to_take)); 

  curr = curr + sizeof(keys_to_take);
  memcpy(curr, keys_ptr, (keys_to_take * sizeof(*keys_ptr)));

  curr = curr + sizeof(*keys_ptr) * NUM_KEY_SLOTS;
  memcpy(curr, children_ptr, sizeof(*children_ptr) * (keys_to_take + 1));

  InternalPageHeader* ph = reinterpret_cast<InternalPageHeader*>(page);
  
  return 1;
};

Result<PageID> InternalPage::GetLeafLeftSibling(Byte* page, PageID pid) {

  // apply binary search to the pageids/childptrs

  InternalPageHeader* page_header = reinterpret_cast<InteralPageHeader*>(page);
  PageID* childptr_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptr_end = childptr_start + page_header->num_keys + 1;
  
  PageID* iter = std::binary_search(childptr_start, childptr_end, pid);

  if (iter == childptr_end) {
    return { .value = 0, .err = ErrType::ChildPtrNotFound };
  };

  if (iter == childptr_start) {
    return { .value = 0, .err = ErrType::LeftLeafSiblingDoesNotExist };
  };

  return { .value = *(iter - 1), .err = ErrType::None };
};

Result<PageID> InternalPage::GetLeafRightSibling(Byte* page, PageID pid) {

  InternalPageHeader* page_header = reinterpret_cast<InteralPageHeader*>(page);
  PageID* childptr_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptr_end = childptr_start + page_header->num_keys + 1;
  
  PageID* iter = std::binary_search(childptr_start, childptr_end, pid);

  if (iter == childptr_end) {
    return { .value = 0, .err = ErrType::ChildPtrNotFound };
  };

  if (iter == childptr_end - 1) {
    return { .value = 0, .err = ErrType::RightLeafSiblingDoesNotExist };
  };

  return { .value = *(iter + 1), .err = ErrType::None };
};

Key* InternalPage::FindKeyFromChildren(Byte* page, PageID left_pid, PageID right_pid) {

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);

  PageID* childptrs_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptrs_end = childptrs_start + page_header->num_keys + 1;

  PageID* iter = std::binary_search(childptrs_start, childptrs_end, left_pid);

  OffsetIndex offset = iter - childptrs_start;
  
  return InternalPage::GetKeysStartPointer(page) +  offset;
};

void InternalPage::SetNewBoundaryKey(Byte* page, Key new_boundary_key, PageID left_pid, PageID right_pid) {

  Key* key_ptr = InternalPage::FindBoundaryKey(page, left_pid, right_pid);
  *key_ptr = new_boundary_key;

};

void InternalPage::DeleteKeyAndChildPtr(Byte* page, PageID merged_page, PageID absorbing_page) {

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);

  Key* key_ptr;
  key_ptr = InternalPage::FindKeyFromChildren(page, absorbing_page, merged_page);

  Key key = *key_ptr;

  Key* keys_start = InternalPage::GetKeysStartPointer(page);
  Key* keys_end = keys_start + page_header->num_keys;

  Key* iter_key = std::binary_search(keys_start, keys_end, key);
  
  if (keys_end - iter_key > 1) memmove(iter_key, iter_key + 1, keys_end - iter_key - 1);

  PageID* childptr_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptr_end = childptr_start + page_header->num_keys + 1;

  PageID* iter_child = std::binary_search(childptr_start, childptr_end, merged_page);

  if (child_ptr - iter_child > 1) memmove(iter_child, iter_child + 1, childptr_end - iter_child - 1);

  return;  
};


bool InternalPage::CheckUnderflow(Byte* page, uint16_t &usedspace) {

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  usedspace = usedspace + (page_header->num_keys * sizeof(Key)) + ((page_header->num_keys + 1) * sizeof(PageID));

  if (usedspace <= INTERNAL_PAGE_UNDERFLOW_THRESHOLD) return true;

  return false;
};


Result<PageID> InternalPage::GetInternalLeftSibling(Byte* page, PageID pid) {

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  PageID* childptr_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptr_end = childptr_start + page_header->num_keys + 1;

  PageID* iter = std::binary_search(childptr_start, childptr_end, pid);

  if (iter == childptr_end) {
    return { .value = 0, .err = ErrType::ChildPtrNotFound };
  };

  if (iter == childptr_start) {
    return { .value = 0, .err = ErrType::LeftInternalSiblingDoesNotExist };
  };

  return { .value = *(iter - 1), .err = ErrType::None };
};

Result<PageID> InternalPage::GetInternalRightSibling(Byte* page, PageID pid) {

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  PageID* childptr_start = InternalPage::GetChildrenStartPointer(page);
  PageID* childptr_end = childptr_start + page_header->num_keys + 1;

  PageID* iter = std::binary_search(childptr_start, childptr_end, pid);

  if (iter == childptr_end) {
    return { .value = 0, .err = ErrType::ChildPtrNotFound };
  };

  if (iter == childptr_end - 1) {
    return { .value = 0, .err = ErrType::RightInternalSiblingDoesNotExist };
  };

  return { .value = *(iter + 1), .err = ErrType::None };
};


BorrowQuery InternalPage::CanLend(PageID pid, uint16_t needed) {
  
  Result<Byte*> page_request = buffer_pool->RequestPage(pid);
  // handle errors

  Byte* page = page_request.value;
  size_t usedspace = InternalPage::GetCurrentUsedSpace(page);
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);

  // pair: key + childptr
  size_t one_pair_size = sizeof(Key) + sizeof(PageID);
  size_t pairs_needed = (needed / one_pair_size) + ((needed % one_pair_size == 0) ? 0 : 1);

  size_t size_loss = one_pair_size * pairs_needed;

  if ((usedspace - size_loss) <= INTERNAL_PAGE_UNDERFLOW_THRESHOLD) {
    return { .can_borrow = false, .borrow_amount = 0, .lender = pid };
  } else {
    return { .can_borrow = true, .borrow_amount = pairs_needed, .lender = pid };
  }
};

// 3rd element is start_ptr + 2 and not start_ptr + 3; check for this mistake below.
void InternalPage::HandleLeftBorrow(Byte* page, PageID borrower_pid, BorrowQuery borrow_report) {

  Key* parent_rotation_key_ptr = InternalPage::FindKeyFromChildren(page, borrow_report.lender, borrower_pid);
  Key parent_rotation_key = *parent_rotation_key_ptr;

  Result<Byte*> borrower_page_request = buffer_pool->RequestPage(borrower_pid);
  // handle errors
  Byte* borrower_page = borrower_page_request.value;

  Result<Byte*> lender_page_request = buffer_pool->RequestPage(borrow_report.lender);
  // handle errors
  Byte* lender_page = lender_page_request.value;

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  InternalPageHeader* borrower_header = reinterpret_cast<InternalPageHeader*>(borrower_page);
  InternalPageHeader* lender_header = reinterpret_cast<InternalPageHeader*>(lender_page);

  Key* borrower_key_start = InternalPage::GetKeyStartPointer(borrower_page);
  Key* borrower_key_end = borrower_key_start + borrower_header->num_keys;
  PageID* borrower_childptr_start = InternalPage::GetChildrenStartPointer(borrower_page);
  PageID* borrower_childptr_end = borrower_childptr_start + borrower_header->num_keys + 1;

  // Move the key array of the borrower back by borrow_amount units
  memmove(borrower_key_start + borrow_report.borrow_amount, borrower_key_start, (borrower_key_end - borrower_key_start) * sizeof(Key));
  // Move the childptr array of the borrower back by borrow_amout units
  memmove(borrower_childptr_start + borrow_report.borrow_amount, borrower_childptr_start, (borrower_childptr_end - borrower_childptr_start) * sizeof(PageID));


  Key* lender_key_start = InternalPage::GetKeyStartPointer(lender_page);
  Key* lender_key_end = lender_key_start + borrower_header->num_keys;
  PageID* lender_childptr_start = InternalPage::GetChildrenStartPointer(lender_page);
  PageID* lender_childptr_end = lender_childptr_start + lender_header->num_keys + 1;

  // Move the last borrow_amount childptrs from the lender to the borrower
  memmove(borrower_childptr_start, (lender_childptr_end - borrow_report.borrow_amount), borrow_report.borrow_amount * sizeof(PageID)); 
  // Move the parent rotating key to the position borrower_key_start + borrow_amount - 1
  memmove(borrower_key_start + (borrow_report.borrow_amount - 1), parent_rotation_key_ptr, sizeof(Key));
  // Move borrow_amount - 1 keys from the end of the lender to the start of the borrower keys array
  memmove(borrower_key_start, lender_key_end - borrow_report.borrow_amount + 1, (borrow_report.borrow_amount - 1) * sizeof(Key));
  // Set the boundary in the parent to the lender key array's (borrow_amount)th entry from the back
  memmove(parent_rotation_key_ptr, lender_key_end - borrow_report.borrow_amount, sizeof(Key));

  borrower_header->num_keys += borrow_report.borrow_amount;
  lender_header->num_keys -= borrow_report.borrow_amount;

  buffer_pool->ReleasePage(borrower_pid, true);
  buffer_pool->ReleasePage(borrow_report.lender, true);
                                              
  return;
};

void InternalPage::HandleRightBorrow(Byte* page, PageID borrower_pid, BorrowQuery borrow_report) {
  
  Key* parent_rotation_key_ptr = InternalPage::FindKeyFromChildren(page, borrower_pid, borrow_report.lender);
  Key parent_rotation_key = *parent_rotation_key_ptr;

  Result<Byte*> borrower_page_request = buffer_pool->RequestPage(borrower_pid);
  // handle errors
  Byte* borrower_page = borrower_page_request.value;

  Result<Byte*> lender_page_request = buffer_pool->RequestPage(borrow_report.lender);
  // handle errors
  Byte* lender_page = lender_page_request.value;

  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  InternalPageHeader* borrower_header = reinterpret_cast<InternalPageHeader*>(borrower_page);
  InternalPageHeader* lender_header = reinterpret_cast<InternalPageHeader*>(lender_page);

  Key* borrower_key_start = InternalPage::GetKeyStartPointer(borrower_page);
  Key* borrower_key_end = borrower_key_start + borrower_header->num_keys;
  PageID* borrower_childptr_start = InternalPage::GetChildrenStartPointer(borrower_page);
  PageID* borrower_childptr_end = borrower_childptr_start + borrower_header->num_keys + 1;

  Key* lender_key_start = InternalPage::GetKeyStartPointer(lender_page);
  Key* lender_key_end = lender_key_start + borrower_header->num_keys;
  PageID* lender_childptr_start = InternalPage::GetChildrenStartPointer(lender_page);
  PageID* lender_childptr_end = lender_childptr_start + lender_header->num_keys + 1;

  // Move childptr from the start of the lender to the end of the borrower.
  memmove(borrower_childptr_end, lender_childptr_start, sizeof(PageID) * borrow_report.borrow_amount);
  // Shift the childptrs of the lender forward.
  memmove(lender_childptr_start, lender_childptr_start + borrow_report.borrow_amount, sizeof(PageID) * (lender_childptr_end - lender_childptr_start - borrow_report.borrow_amount));

  // Move the key from parent to the end of the keys in borrower.
  memmove(borrower_key_end, parent_rotation_key_ptr, sizeof(Key));
  // Move borrow_amount - 1 keys from the start of the lender's key array to the position borrower_key_end + 1.
  memmove(borrower_key_end+1, lender_key_start, sizeof(Key) * (borrow_result.borrow_amount - 1));
  // Move the lender_key_start + borrow_amount key to the parent rotation key place.
  memmove(parent_rotation_key_ptr, lender_key_start + borrow_report.borrow_amount - 1, sizeof(Key));

  // Shifting the keys forward in the lender
  memmove(lender_key_start, lender_key_start + borrow_report.borrow_amount, sizeof(Key) * (lender_key_end - lender_key_start - borrow_report.borrow_amount));


  borrower_header->num_keys += borrow_report.borrow_amount;
  lender_header->num_keys -= borrow_report.borrow_amount;

  buffer_pool->ReleasePage(borrower_pid, true);
  buffer_pool->ReleasePage(borrow_report.lender, true);
                                              
  return;
};
