#pragma once
#include "../../../../commons/types.h"
#include "../../../../commons/constants.h"
#include "./page.h"
#include <cstring>
#include <algorithm>
#include <iostream>

constexpr uint16_t INTERNAL_PAGE_HEADER_SIZE = 11;
constexpr uint16_t NUM_KEY_SLOTS = 254;
constexpr uint16_t NUM_CHILD_PAGEID_SLOTS = 255;
constexpr uint16_t KEY_SIZE = 8;
constexpr uint16_t CHILD_PTR_SIZE = 8;

// this is defined in data_size as well and not count
constexpr uint16_t INTERNAL_UNDERFLOW_THRESHOLD = ((PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / 2) - KEY_SIZE - CHILD_PTR_SIZE;

struct __attribute__((__packed__)) InternalPageHeader {
  PageType page_type;
  PageID page_id;
  uint16_t num_keys;
};

namespace InternalPage {
  PageID GetChildPageID(Byte* page, Key key);
  uint16_t CheckUsedSpace(Byte* page);
  Bool CheckSlotAvailable(Byte* page);
  Key* GetKeysStartPointer(Byte* page);
  PageID* GetChildrenStartPointer(Byte* page);
  PageID GetValueAtIndex(Byte* page, OffsetIndex index);
  Key HandleSplit(Byte* old_page, Byte* new_page, Key key_to_insert, PageID page_id_to_insert, PageID new_page_id);
  Bool InsertKeyValue(Byte* page, Key boundary_key, PageID new_pid);
  Bool MakePage(Byte* page, Key* keys_ptr, PageID* children_ptr, uint16_t keys_to_take, PageID pid); 
  Key* FindKeyFromChildren(Byte* page, PageID left_pid, PageID right_pid);
  void SetNewBoundaryKey(Byte* page, Key new_boundary_key, PageID left_pid, PageID right_pid);
  bool CheckUnderflow(Byte* page, uint16_t &usedspace);
  Result<PageID> GetLeafLeftSibling(Byte* page, PageID pid);
  Result<PageID> GetLeafRightSibling(Byte* page, PageID pid);
  Result<PageID> GetInternalLeftSibling(Byte* page, PageID pid);
  Result<PageID> GetInternalRightSibling(Byte* page, PageID pid);
  BorrowQuery CanLend(Byte* page, uint16_t needed);
  void HandleLeftBorrow(Byte* page, Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report);
  void HandleRightBorrow(Byte* page, Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report);
  Key DeletePartitionKeyAndChildPtr(Byte* page, PageID left_child_pid, PageID right_child_pid);
  void DeleteKeyAndChildPtr(Byte* page, PageID absorbing_page, PageID merged_page);
  void MergePages(Key partition_key, Byte* absorber_page, Byte* absorbee_page);
  void DumpPage(Byte* page);
};
