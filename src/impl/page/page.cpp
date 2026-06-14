
#include <./page.h>
#include <../../commons/utils.h>
#include <../../commons/types.h>


Bool CheckSlotAvailable(Byte* page, uint16_t key_size) {
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
  return GetKeysStartPointer(page) + NUM_KEY_SLOTS;
};

PageID InternalPage::GetValueAtIndex(Byte* page, OffsetIndex index) {
  PageID* value_start = GetChildrenStartPointer(page);
  PageID res = *(value_start + index);
  return res;
};

PageID InternalPage::GetChildPageID(Byte* page, uint16_t key) {
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  Key* key_start = GetKeysStartPointer(page);
  Key* key_end = key_start + page_header->num_keys;

  Key* it = std::upper_bound(key_start, key_end, key);
  OffsetIndex index_child_ptr = it - key_start;

  return GetValueAtIndex(page, index_child_ptr);
};

// inserts another key inside the internal page at appropriate location and its corresponding child pid at the proper location.
Bool InternalPage::InsertKeyValue(Byte* page, Key boundary_key, PageID new_pid) {
    
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(page);
  Key* key_start = GetKeysStartPointer(page);
  Key* key_end = key_start + page_header->num_keys;
  Key* it = std::upper_bound(key_start, key_end, boundary_key);
  OffsetIndex index_child_ptr = it - key_start;

  memmove(it+1, it, (key_end - it) * KEY_SIZE);

  *it = boundary_key;
  OffsetIndex insertion_index = index_child_ptr + 1;

  PageID* value_start = GetChildrenStartPointer(page);
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
  PageID temp_ptr[NUM_CHILD_PAGEID_SLOTS+1];
  InternalPageHeader* page_header = reinterpret_cast<InternalPageHeader*>(old_page);
  Key* key_start = GetKeysStartPointer(page);
  Key* ptr_start = GetChildrenStartPointer(page);

  memcpy(temp_keys, key_start, page_header->num_keys * KEY_SIZE);
  memcpy(temp_ptr, ptr_start, (page_header->num_keys + 1) * CHILD_PTR_SIZE);

  Key* it = std::upper_bound(temp_keys, temp_keys + page_header->num_keys, key_to_insert);
  
  if (it != temp_keys + page_header->num_keys) {
    memmove(it + 1, it, (temp_keys + page_header->num_keys - it) * KEY_SIZE);
  };

  *it = key_to_insert;
  OffsetIndex index_child_ptr = it - temp_keys;
  OffsetIndex insertion_index = index_child_ptr + 1;

  PageID* insertion_it = temp_ptr + insertion_index;

  if (insertion_it != temp_ptr + page_header->num_keys + 1) {
    memmove(insertion_it+1, insertion_it, ((temp_ptr + page_header->num_keys + 1) - insertion_it) * sizeof(PageID));
  };

  *insertion_it = page_id_to_insert;

  // size of temp_keys = page_header->num_keys + 1
  // size of temp_ptr = page_header->num_keys + 2
  

  // 1 2 3 4 5
  // 1 2 3 4 5 6

  Key* boundary_key = temp_keys + ((page_header->num_keys + 1) / 2);
  InternalPage::MakePage(old_page, temp_keys, temp_ptr, ((page_header->num_keys + 1) / 2), page_header->page_id, page_header->parent_pid);
  InternalPage::MakePage(new_page, boundary_key + 1, temp_ptr + ((page_header->num_keys + 1) / 2) + 1, ((page_header->num_keys) / 2), page_id_to_insert, page_header->parent_pid);

  return *boundary_key;  
};


Bool InsernalPage::MakePage(Byte* page, Key* keys_ptr, PageID* children_ptr, uint16_t keys_to_take, PageID pid, PageID parent_pid) {
  
  Byte* curr = page;
  *curr = PageType::InternalPage;

  curr = curr + sizeof(PageType::InternalPage);
  memcpy(curr, &pid, sizeof(value));

  curr = curr + sizeof(pid);
  memcpy(curr, &keys_to_take, sizeof(keys_to_take)); 

  curr = curr + sizeof(keys_to_take);
  memcpy(curr, &parent_pid, size(parent_pid));
  
  curr = curr + sizeof(parent_pid);
  memcpy(curr, keys_ptr, sizeof(*keys_ptr) * keys_to_take);

  curr = curr + sizeof(*keys_ptr) * NUM_KEY_SLOTS;
  memcpy(curr, children_ptr, sizeof(*children_ptr) * (keys_to_take + 1));
  
  return 1;
};

uint16_t LeafPage::CheckAvailableSpace(Byte* page) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  Byte* free_space_start = page;
  Byte* free_space_end = page + page_header->free_space_end_offset;
  free_space_start = free_space_start + LEAF_PAGE_HEADER_SIZE;
  free_space_start = free_space_start + (page_header->slot_array_size * SLOT_SIZE);

  uint16_t free_bytes = free_space_end - free_space_start + 1;
  return free_bytes;
};

SlotArrayElement* upper_bound(const SlotArrayElement* start, const SlotArrayElement* end, Byte* page, Key x) {

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


RecordID LeafPage::InsertTuple(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key) {

  // buffer here is all the data for one tuple we might need to divide this into multiple tuples

  if (buffer_size + TUPLE_HEADER_SIZE <= TUPLE_SIZE_LIMIT) {
    // no overflow

    // make the tuple and place it.
    // return the record id to parent.


  } else {
    // overflow
    //
    // take TUPLE_SIZE_LIMIT - TUPLE_HEADER_SIZE bytes from the buffer
    // make tuple out of them
    // place the tuple
    // request an overflow page
    // recursive call to the next page with the remaining buffer data.
    // set the overflow flag in current tuple head and set the overflow_address to the child's returned recordid

  };

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* slot_array_start = page + LEAF_PAGE_HEADER_SIZE;
  SlotArrayElement* slot_array_end = page + (SLOT_SIZE * page_header->slot_array_size);

  SlotArrayElement* it = upper_bound(slot_array_start, slot_array_end, page, key);
  memmove(it+SLOT_SIZE, it, (slot_array_end - it) * SLOT_SIZE);




};



