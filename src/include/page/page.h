#include <../../commons/types.h>
#include <../../commons/constants.h>
#include <../../commons/uitls.h>
#include <cstring>

// We will need serializer for storing struct into file because if we store directly 
// the padding will be addded too and Page wont remain the size it is supposed to be.
//
// Page will have a slot array and not log structured type.
// 
// [Header][Data Slot Array].....[FREE SPACE].....[Tuple n][Tuple n-1]...[Tuple 1]
//
// Slot Array entry will store: {slot offset (address), slot size, is_deleted}
//
// Header can track whether where the free space in the middle starts and ends
//
// Also we will make the page tuples compact on delete and not just run compact ocassionally like many other databases do.
//
// If we make the tuple not compact on delete, the delete operation becomes cheap. But between the time of delete and the next compact
// the slot can only be used by new data whose size is smaller than the whole size.
//
// With this approach the address of the tuple becomes (page # + slot #)
// 
// We have to keep word alignment in mind while we make the structure for the tuple.
// OS like to read and write in specific chunks like 64 bits.
//
// Buffer Pool
// Page Table tells us about what page is where in buffer pool.
//
// Decided on the clustered database implementation.
// So the BTree will have 2 types of pages:
//     1) Leaf Pages/Nodes containing the actual data
//     2) Internal Pages/Nodes for traversing the BTree.
// 

struct PageHeader {
  // page type
  PageType page_type;
  // page id
  PageID page_id;
};

constexpr LEAF_PAGE_HEADER_SIZE = 13;
constexpr SLOT_SIZE = 4;

struct LeafPageHeader {
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
  PageID prev_pid;
  // sibling next pageid
  PageID next_pid;
  // parent pageid
  PageID parent_pid;
};

constexpr INTERNAL_PAGE_HEADER_SIZE = 7;
constexpr NUM_KEY_SLOTS = 1021;
constexpr NUM_CHILD_PAGEID_SLOTS = 1022;
constexpr KEY_SIZE = 2;
constexpr CHILD_PTR_SIZE = 2;

struct InternalPageHeader {
  // page type
  PageType page_type;
  // page id
  PageID page_id;
  // slot array size
  uint16_t num_keys;
  // parent pageid
  PageID parent_pid;
};

struct __attribute__((__packed__)) SlotArrayElement {
  Offset offset;
  uint16_t length;
  // Not clear if we really need this right now since we run compact after every delete
  // Byte is_deleted;
};

class LeafPage {
  // SlotArrayElement* slot_array;
  // data : slot array | free space | tuples
  RecordID InsertTuple(Byte* page, const Byte *buffer, BufferSize buffer_size, Key key);
  Bool HandleSplit(Byte* page, Byte* new_page, Byte* buffer, BufferSize buffer_size);
  uint16_t CheckAvailableSpace(Byte* page);
};

class InternalPage {
  // takes the pointer to first byte of the page and the key and return page_id of the page associated with that key in the internal page.
  PageID GetChildPageID(Byte* page, uint16_t index);

  Bool CheckSlotAvailable(Byte* page, uint16_t key_size);
  Key* GetKeysStartPointer(Byte* page);
  PageID* GetChildrenStartPointer(Byte* page);
  PageID GetValueAtIndex(Byte* page, OffsetIndex index);
  // Splits the keys in 
  uint16_t HandleSplit(Byte* old_page, Byte* new_page, Key key_to_insert, PageID page_id_to_insert);
  Bool InsertKeyValue(Byte* page, Key boundary_key, PageID new_pid);
  Bool MakePage(Byte* page, Key* keys_ptr, PageID* children_ptr, uint16_t keys_to_take, PageID pid, PageID parent_pid);
};


