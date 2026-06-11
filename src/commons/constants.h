#pragma once

#include <cstddef>
#include <string>

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t GENERAL_PAGE_HEADER_SIZE = 5;
constexpr size_t TABLE_PAGE_HEADER_SIZE = 11;
constexpr size_t TABLE_PAGE_DATA_SIZE = 4008;
constexpr std::string DATA_DIR = "./data";
constexpr std::string DB_PATH  = "./data/engine.db";
constexpr std::string LOG_PATH = "./data/engine.log";
constexpr size_t POOL_SIZE = 100;
constexpr size_t BUFFER_FRAME_META_SIZE = 5;
