#pragma once

#include <cstddef>
#include <cstdint>

using PageID = uint16_t;
using SlotID = uint16_t;

using Key = uint16_t;
using Offset = uint16_t;
using OffsetIndex = uint16_t;

using TupleLength = uint16_t;
using AttributeCount = uint16_t;
using Byte = std::byte;
using DeleteStatus = bool;
using BitmapSize = uint16_t;
using OperationStatus = bool;
using Bool = uint8_t;

using BufferSize = uint16_t;

struct __attribute__((__packed__)) RecordID {
  PageID pid;
  OffsetIndex slot_index;
};

struct __attribute__((__packed__)) SplitReport {
  Bool was_split;
  PageID new_page_id;
  Key boundary_key;
};

struct __attribute__((__packed__)) NewPage {
  Byte* ptr;
  PageID pid;
};

struct __attribute__((__packed__)) SearchResult {
  TupleLength size;
  Byte* ptr;
};

enum ErrType {

  None,
  DefaultErr,
  SystemErr,

  // Storage Manager Read
  FileCorruption,

  // Storage Manager Write
  DiskFullOrTruncated,
  
  BufferPoolFull,
  AllPagesPinned,
  PageNotFoundInBufferPool,
};

enum class PageType : uint8_t {
    Meta = 1,
    InternalPage = 2,
    LeafPage = 3,
    OverflowPage = 4,
};

template <typename T>
struct Result {
    T value;
    ErrType err;
};
