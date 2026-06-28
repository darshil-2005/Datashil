#include "../../include/utils/utils.h"

uint32_t Utils::CalculateHash(const Byte *data, size_t length) {
  uint32_t hash = 0x811c9dc5;
  for (size_t i = 0; i < length; ++i) {
    hash ^= static_cast<uint32_t>(data[i]);
    hash *= 0x01000193;
  };
  return hash;
};

void Utils::SetChecksum(Byte *struct_ptr, size_t total_size) {
  size_t data_length = total_size - sizeof(uint32_t);
  uint32_t checksum = Utils::CalculateHash(struct_ptr, data_length);
  std::memcpy(struct_ptr + data_length, &checksum, sizeof(uint32_t));
};

bool Utils::VerifyChecksum(const Byte *struct_ptr, size_t total_size) {
  size_t data_length = total_size - sizeof(uint32_t);
  uint32_t calculated = Utils::CalculateHash(struct_ptr, data_length);
  uint32_t provided = 0;
  std::memcpy(&provided, struct_ptr + data_length, sizeof(uint32_t));
  return calculated == provided;
};
