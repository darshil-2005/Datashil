
/* 
 * B Tree Implementation Plan
 *
 * Problems:
 * When to split the node and when to put the tuples into another page?
 * Might set a size_limit on a tuple if it exceeds that we break it up and store it separately.
 *
 *
 *
 * 
 * class BTree:
 * Intializes with information on what is the primary key.
 * Only internal nodes and leaf nodes.
 * We store the page_id of the root node in another meta page that is not connected to the tree.
 *
 * Attributes:
 *
 *  
 * Methods: 
 * RetrieveTuple(): might look into returning a file stream here for large blobs later.
 * InsertTuple(): we might need to split here.
 * DeleteTuple()
 *
 * splitLeafNode()
 * splitInternalNode()
 * mergeLeafNode()
 * mergeInternalNode()
 *
 */

#include <../../commons/types.h>
#include <../../include/bufferpool/bufferpool.h>
#include <../../include/page/page.h>

class BTree {
  
  PageID root_page_id;
  BufferPool* buffer_pool;
  // Some variable that holds info about the key or maybe it shall be passed directly to the functions.
  // for this implementation we will assume the primary key is always 2 bytes and only allow indexing on those sizes.
  // The primary key will be kept immutable (both value and type).
  

  BTree(BufferPool& bf);
  Result<RecordID> InsertTuple(const Byte* buffer, BufferSize buffer_size, const uint16_t index);
};

