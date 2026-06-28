#include "../../../types.h"
#include <cstdint>
#include <cstring>

namespace Utils {
  uint32_t CalculateHash(const Byte* data, size_t length);
  bool VerifyChecksum(const Byte* struct_ptr, size_t total_size);
  void SetChecksum(Byte* struct_ptr, size_t total_size);
};

