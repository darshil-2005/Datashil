#pragma once
#include "../../commons/types.h"
#include "../../commons/constants.h"
#include "./page.h"
#include <cstring>
#include <algorithm>

constexpr uint16_t INTERNAL_PAGE_HEADER_SIZE = 5;
constexpr uint16_t NUM_KEY_SLOTS = 1022;
constexpr uint16_t NUM_CHILD_PAGEID_SLOTS = 1023;
constexpr uint16_t KEY_SIZE = 2;
constexpr uint16_t CHILD_PTR_SIZE = 2;

struct __attribute__((__packed__)) InternalPageHeader {
  // page type
  PageType page_type;
  // page id
  PageID page_id;
  // slot array size
  uint16_t num_keys;
};

namespace InternalPage {
  // takes the pointer to first byte of the page and the key and return page_id of the page associated with that key in the internal page.
  PageID GetChildPageID(Byte* page, Key key);

  Bool CheckSlotAvailable(Byte* page, uint16_t key_size);
  Key* GetKeysStartPointer(Byte* page);
  PageID* GetChildrenStartPointer(Byte* page);
  PageID GetValueAtIndex(Byte* page, OffsetIndex index);
  // Splits the keys in 
  uint16_t HandleSplit(Byte* old_page, Byte* new_page, Key key_to_insert, PageID page_id_to_insert);
  Bool InsertKeyValue(Byte* page, Key boundary_key, PageID new_pid);
  Bool MakePage(Byte* page, Key* keys_ptr, PageID* children_ptr, uint16_t keys_to_take, PageID pid); 
};
