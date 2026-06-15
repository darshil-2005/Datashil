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
  
  return 1;
};

