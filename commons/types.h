#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "./constants.h"

using PageID = uint64_t;
using SlotID = uint16_t;

using Key = uint64_t;
using Offset = uint64_t;
using PageOffset = uint16_t;
using OffsetIndex = uint16_t;

using TupleLength = uint16_t;
using AttributeCount = uint16_t;
using Byte = std::byte;
using BitmapSize = uint16_t;
using OperationStatus = bool;
using Bool = uint8_t;

using BufferSize = uint64_t;

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

enum class PageType : uint8_t {
    MetaPage = 1,
    InternalPage = 2,
    LeafPage = 3,
    OverflowPage = 4,
    InvalidPage = 5,
};

enum class Command : uint8_t {
  Invalid = 0,
  Search = 1,
  Insert = 2,
  Delete = 3,
  Quit = 4
};

struct Query {
  bool valid;
  Command command;
  Key key;
  std::vector<std::string> payload;

  Query() : valid(false), command(Command::Invalid) {};  
};

struct WriteStatus {
  uint16_t written;
  Byte* overflow_info_store_address;
};

struct DeleteStatus {
   bool underflown;
   PageType page_type;
   uint16_t current_size;
   bool success;
};

struct BorrowQuery {
  bool can_borrow;
  uint16_t borrow_amount;
};

struct __attribute__((__packed__)) RequestHeader {
  uint32_t magic_number;
  uint32_t total_length;
  Command command;
  uint32_t header_checksum;
};

struct __attribute__((__packed__)) ResponseHeader {
  uint32_t magic_number;
  uint32_t total_length;
  uint8_t status_code;
  Command echo_command;
  uint32_t header_checksum;
};

struct Response {
  ResponseHeader header;
  std::vector<Byte> payload;

  Response() {
    header.echo_command = Command::Invalid;
    header.magic_number = NETWORK_MAGIC_NUMBER;
    header.header_checksum = 0;
    header.status_code = 4;
    header.total_length = sizeof(ResponseHeader);
  };

  Response(ResponseHeader response_header, std::vector<Byte> payload) : header(response_header), payload(std::move(payload)) {};
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

  ChildPtrNotFound,

  RightInternalSiblingDoesNotExist, 
  LeftInternalSiblingDoesNotExist,
  
  RightLeafSiblingDoesNotExist, 
  LeftLeafSiblingDoesNotExist,

  OperationNotAllowed,
};


template <typename T>
struct Result {
    T value;
    ErrType err;
};
