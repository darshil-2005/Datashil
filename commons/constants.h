#pragma once

#include <cstddef>
#include <string>
#include <cstdint>

constexpr size_t POOL_SIZE = 1000;
constexpr size_t PAGE_SIZE = 4096;

constexpr size_t STORAGE_MANAGER_META_PAGE_ID = 0;
constexpr size_t DATABASE_META_PAGE_ID = 1;

constexpr uint32_t STORAGE_MANAGER_MAGIC_NUMBER = 0x34ABE72C;
constexpr uint32_t DATABASE_MAGIC_NUMBER = 0x67AD7CB3;
constexpr uint32_t NETWORK_MAGIC_NUMBER = 0x8DED1CB2;
constexpr size_t MAGIC_NUMBER_SIZE = sizeof(uint32_t);
