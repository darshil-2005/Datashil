#pragma once
#include "../../commons/types.h"
#include "../../commons/constants.h"
#include "./page.h"
#include <cstring>
#include <algorithm>

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

constexpr uint16_t LEAF_PAGE_HEADER_SIZE = 11;
constexpr uint16_t SLOT_SIZE = 4;
constexpr uint16_t OVERFLOW_PAGE_HEADER_SIZE = 6;
constexpr uint16_t OVERFLOW_PAGE_OVERFLOW_INFO_OFFSET = 3;
constexpr uint16_t OVERFLOW_TUPLE_DATA_SIZE_MAX = 1280;
constexpr uint16_t TUPLE_HEADER_SIZE = 3;
constexpr uint16_t MIN_LEAF_PAGE_DATA = 128 - TUPLE_HEADER_SIZE - SLOT_SIZE;
constexpr uint16_t MAX_LEAF_PAGE_DATA = 1280 - TUPLE_HEADER_SIZE - SLOT_SIZE;

struct __attribute__((__packed__)) OverflowInfo {
  Bool overflow;
  PageID overflow_page;
};
  
struct __attribute__((__packed__)) SlotArrayElement {
  Offset offset;
  uint16_t length;
};

struct __attribute__((__packed__)) OverflowPageHeader {
  PageType page_type;
  PageID page_id;
  Bool overflow;
  PageID overflow_page;
};

struct __attribute__((__packed__)) LeafPageHeader {
  // page type
  PageType page_type;
  // page id
  PageID page_id;
  // free space end
  // num of bytes from the start of the page to the last free byte.
  Offset free_space_end_offset;
  // slot array size
  uint16_t slot_array_size;
  // sibling prev pageid
  PageID left_pid;
  // sibling next pageid
  PageID right_pid;
};

namespace LeafPage {
  // SlotArrayElement* slot_array;
  // data : slot array | free space | tuples
  RecordID InsertTuple(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key);
  Key HandleSplit(Byte* page, Byte* new_page);
  bool MakePage(Byte* page, SlotArrayElement* slot_array_start, uint16_t slot_array_size, Byte* buffer, PageID pid, PageID left_pid, PageID right_pid);
  uint16_t CheckAvailableSpace(Byte* page);
  SearchResult Search(Byte* page, Key key);
};
