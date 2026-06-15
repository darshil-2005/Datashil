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

Key LeafPage::HandleSplit(Byte* old_page, Byte* new_page, PageID new_pid) {

  Byte* buffer[PAGE_SIZE];
  memmove(buffer, old_page, PAGE_SIZE);

  uint16_t threshold = (PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / 2;
  uint16_t consumed_space = 0;

  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(buffer);
  SlotArrayElement* slot_array_start = old_page + LEAF_PAGE_HEADER_SIZE;
  SlotArrayElement* slot_array_new_page_start = old_page + LEAF_PAGE_HEADER_SIZE;

  for (int i=0; i < page_header->slot_array_size; i++) {
    SlotArrayElement* current = slot_array_start[i];
    consumed_space = consumed_space + current->length;
    if (consumed_space > threshold) {
      slot_array_new_page_start = current + 1;      
    };
  };

  LeafPage::MakePage(old_page, slot_array_start, 
      (uint16_t)(slot_array_new_page_start - slot_array_start),
      buffer, page_header->page_id, page_header->left_pid, new_pid);

  LeafPage::MakePage(new_page, slot_array_new_page_start, 
      (uint16_t)((slot_array_start + page_header->slot_array_size) - slot_array_new_page_start),
      buffer, new_pid, page_header->page_id, page_header->right_id);

  Key boundary_key = LeafPage::GetKeyFromSlotElement(buffer, slot_array_new_page_start);
  return boundary_key;
};

bool LeafPage::MakePage(Byte* page, SlotArrayElement* slot_array_start, uint16_t slot_array_size, Byte* buffer, PageID pid, PageID left_pid, PageID right_pid) {

  Offset free_space_end_offset = PAGE_SIZE - 1;
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);

  page_header->slot_array_size = slot_array_size;
  page_header->page_type = PageType::LeafPage;
  page_header->page_id = pid;
  page_header->left_pid = left_pid;
  page_header->right_pid = right_pid;

  SlotArrayElement* slot_array = reinterpret_cast<SlotArrayElement*>(page + LEAF_PAGE_HEADER_SIZE);

  for (int i=0; i<slot_array_size; i++) {
    Offset offset = slot_array_start[i]->offset;
    TupleLength length = slot_array_start[i]->length;

    memcpy(page + free_space_end_offset - length + 1, buffer + offset, length); 
    slot_array[i]->offset = free_space_end_offset - length + 1;
    slot_array[i]->length = length;
    free_space_end_offset = slot_array[i]->offset;
  };

  page_header->free_space_end_offset = free_space_end_offset;
  
  return true;
};

SlotArrayElement* lower_bound(const SlotArrayElement* start, const SlotArrayElement* end, Byte* page, Key x) {

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

Key GetKeyFromSlotElement(Byte* page, SlotArrayElement* element) {
  uint16_t key;
  Byte* key_address = page + element->offset + TUPLE_HEADER_SIZE;
  memcpy(&key, key_address, sizeof(uint16_t));
  return key;
};

SearchResult LeafPage::Search(Byte* page, Key key) {
  
  LeafPageHeader* page_header = reinterpret_cast<LeafPageHeader*>(page);
  SlotArrayElement* slot_array_start = page + LEAF_PAGE_HEADER_SIZE;
  SlotArrayElement* slot_array_end = slot_array_start + page_header->slot_array_size;
  SlotArrayElement* search_result = lower_bound(slot_array_start, slot_array_end, page, key);

  if (search_result == slot_array_end) {
    return { .size = 0, .ptr = nullptr };
  };

  Key returned_key = LeafPage::GetKeyFromSlotElement(page, search_result);

  if (returned_key == key) {
    return { .size = search_result->length, .ptr = page + search_result->offset };
  };

  return { .size = 0, .ptr = nullptr };
};



