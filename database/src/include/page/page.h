#pragma once
#include "../../../../commons/types.h"
#include "../../../../commons/constants.h"

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

struct __attribute__((__packed__)) PageHeader {
  // page type
  PageType page_type;
  // page id
  PageID page_id;
};
