
#include <../../commons/types.h>

struct TupleHeader {
  AttributeCount attribute_count;
  TupleLength size; 
};
  
class Tuple {
  TupleHeader header;
  Byte* Bitmap;
  BitmapSize bitmap_size;
  Byte* data;
  size_t getBitmapSize() const;
};
