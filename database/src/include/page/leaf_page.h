#pragma once
#include "../../../../commons/types.h"
#include "../../../../commons/constants.h"
#include "./page.h"
#include "../bufferpool/bufferpool.h"
#include <cstring>
#include <algorithm>
#include <iostream>

// Notes: 
// Can implement streaming for very large tuples.
// The read from the memory can be made significantly faster if the tuples are memory aligned.
//     1) Because right now we use memcpy which can take 2 cpu instructions to get 1 byte of data if data is unaligned.
//     2) reinterpret_cast causes fault when fetching unaligned memory on some ARM processors and trigger an intricate time
//        consuming mechanism on most intel precessors to stick together the required memory bytes.


/*
 * This might need complete change if I decide to implement the column swapping with padding mentioned above in the future
 * or maybe we can deal with that at the time table schema is created.
 *
 * Element of column_offsets represent the byte at which a specific column ends.
 * I am not allowing schema change in this implementation, but if i had to:
 * We have 2 choices for schema change:
 *     1) A full rewrite after the schema is altered of all entries.
 *     2) Maintain each version of schema and a schema_version variable in the tuple header indicating which schema version 
 *        this tuple uses and parse the tuple according to that.
 *
 */

constexpr uint16_t LEAF_PAGE_HEADER_SIZE = 31;
constexpr uint16_t SLOT_SIZE = 5;
constexpr uint16_t OVERFLOW_PAGE_HEADER_SIZE = 18;
constexpr uint16_t OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET = 9;
constexpr uint16_t TUPLE_HEADER_SIZE = 13;
constexpr uint16_t MIN_LEAF_PAGE_DATA = 128;

// Page underflow if it is less than or equal to this.
// This threshold makes sure we can always merge underflow node and a node that will underflow if we borrow from it.
constexpr uint16_t LEAF_UNDERFLOW_THRESHOLD = 2 * (PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / 5;

// The tuple overflow if the size of its payload is bigger than this. The slot size and the tuple header size
// are subtracted because this threshold looks at the payload only.
constexpr uint16_t MAX_LEAF_PAGE_DATA = (LEAF_UNDERFLOW_THRESHOLD / 2);

struct __attribute__((__packed__)) OverflowInfo {
  Bool overflow;
  PageID overflow_page;
};
  
struct __attribute__((__packed__)) TupleHeader {
  Bool overflow;
  PageID overflow_page;
  uint32_t size;
};

struct __attribute__((__packed__)) SlotArrayElement {
  Bool is_deleted;
  PageOffset offset;
  uint16_t length;
};

struct __attribute__((__packed__)) OverflowPageHeader {
  PageType page_type;
  PageID page_id;
  Bool overflow;
  PageID overflow_page;
};

struct __attribute__((__packed__)) LeafPageHeader {
  PageType page_type;
  PageID page_id;
  PageOffset free_space_end_offset;
  uint16_t slot_array_size;
  uint16_t garbage_bytes;
  PageID left_pid;
  PageID right_pid;
};

struct SearchResult {
  bool found;
  Offset tuple_offset;
  Offset tuple_end_offset;
  size_t total_tuple_size;
  bool overflow;
  PageID overflow_page_id;
};

class PayloadStream {
  private:
    PageID curr_pid;
    Offset curr_page_offset;
    Offset curr_page_end;
    size_t total_bytes;
    size_t bytes_read;
    bool overflow;
    PageID overflow_page_id;
    BufferPool* buffer_pool;
    Offset ReadPage(const Byte* page, Byte* buffer, size_t n, Offset start_offset, Offset max_offset);

  public:
    PayloadStream();
    PayloadStream(BufferPool *bf, PageID leaf_pid, Offset tuple_offset, Offset tuple_end_offset, size_t total_tuple_size, bool overflow, PageID overflow_page_id);
    size_t NextBytes(Byte* buffer, size_t n);
    bool IsEOF() const { return bytes_read >= total_bytes; }
    size_t PayloadRemaining();
};

namespace LeafPage {
  Key HandleSplit(Byte* page, Byte* new_page, PageID pid);
  bool MakePage(Byte* page, SlotArrayElement* slot_array_start, uint16_t slot_array_size, Byte* buffer, PageID pid, PageID left_pid, PageID right_pid);
  SearchResult Search(Byte* page, Key key);
  SlotArrayElement* upper_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x);
  SlotArrayElement* lower_bound(SlotArrayElement* start, SlotArrayElement* end, Byte* page, Key x);
  Key GetKeyFromSlotElement(Byte* page, SlotArrayElement* element); 
  Offset ReadPage(Byte* page, Byte* buffer, size_t n, Offset start_offset, Offset max_offset);
  uint32_t GetTupleSizeFromSlotElement(Byte* page, SlotArrayElement* element);
  WriteStatus WriteChunkLeaf(Byte* page, const Byte* buffer, BufferSize buffer_size, Key key, TupleHeader* default_tuple = nullptr);
  WriteStatus WriteChunkOverflow(Byte* page, const Byte* buffer, BufferSize buffer_size);
  uint16_t GetCurrentUsedSpace(Byte* page);
  BorrowQuery CanLendFromRight(Byte* page, uint16_t needed);
  BorrowQuery CanLendFromLeft(Byte* page, uint16_t needed);
  Key HandleLeftBorrow(Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report);
  Key HandleRightBorrow(Byte* borrower_page, Byte* lender_page, BorrowQuery borrow_report);
  uint16_t CheckAvailableSpace(Byte* page);
  uint16_t CheckGarbageBytes(Byte* page);
  bool DeleteTuple(Byte* page, Key key);
  void MergePages(Byte* to_page, Byte* from_page);
  void DefragmentPage(Byte* page);
  Key GetPageFirstKey(Byte* page);
  void DumpPage(Byte* page);
  void DumpPageFirstLast(Byte *page);
};

