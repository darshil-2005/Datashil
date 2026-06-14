#include <../../commons/types.h>

// Notes: 
// Can implement streaming for very large tuples.
// The read from the memory can be made significantly faster if the tuples are memory aligned.
//     1) Because right now we use memcpy which can take 2 cpu instructions to get 1 byte of data if data is unaligned.
//     2) reinterpret_cast causes fault when fetching unaligned memory on some ARM processors and trigger an intricate time
//        consuming mechanism on most intel precessors to stick together the required memory bytes.



struct __attribute__((__packed__)) TupleHeader {
  uint16_t attribute_count;
  uint16_t size; 
  Bool overflow;
  RecordID overflow_address;
};
  
class Tuple {
  TupleHeader header;
  uint16_t null_bitmap_size;
  Byte* null_bitmap;

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
   *
   */

  Byte* column_offsets;
  Byte* data;
  size_t getBitmapSize(uint_t attribute_count) const;
};
