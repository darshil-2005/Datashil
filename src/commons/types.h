#pragma once

#include <cstddef>
#include <cstdint>

using PageID = uint16_t;
using SlotID = uint16_t;

using Offset = uint16_t;
using OffsetIndex - uint16_t;

using SlotLength = uint16_t;
using TupleLength = uint16_t;
using AttributeCount = uint16_t;
using Byte = std::byte;
using DeleteStatus = bool;
using BitmapSize = uint16_t;
using OperationStatus = bool;
using Bool = uint8_t;

enum ErrType {

  None,
  DefaultErr,
  SystemErr,

  // Storage Manager Read
  PageCorruption,

  // Storage Manager Write
  DiskFullOrTruncated,
  
  BufferPoolFull,
  AllPagesPinned,
};

enum class PageType : uint8_t {
    Invalid = 0,
    Meta = 1,
    BPlusInternal = 2,
    BPlusLeaf = 3,
    Table = 4
};

template <typename T>
struct Result {
    T value;
    ErrType err;
};
