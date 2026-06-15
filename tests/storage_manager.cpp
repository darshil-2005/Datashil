#include "./utils/catch.hpp"
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include "../src/include/storageManager/storageManager.h"

namespace fs = std::filesystem;

Byte* AllocateAlignedBuffer() {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, PAGE_SIZE) != 0) {
        throw std::bad_alloc();
    }
    std::memset(ptr, 0, PAGE_SIZE);
    return static_cast<Byte*>(ptr);
}

TEST_CASE("StorageManager Handles Disk I/O and Allocation", "[storage]") {
    if (fs::exists(DATA_DIR)) {
        fs::remove_all(DATA_DIR);
    }
    
    StorageManager sm;
    bool boot_success = sm.Bootstrap();
    REQUIRE(boot_success == true);

    SECTION("Bootstrap initializes Page 0 metadata correctly") {
        REQUIRE(sm.GetNewPageOffsetIndex() == 2);
    }

    SECTION("AllocateNewPage increments the page index") {
        uint16_t initial_index = sm.GetNewPageOffsetIndex();
        
        Result<PageID> alloc_result = sm.AllocateNewPage();
        
        REQUIRE(alloc_result.err == ErrType::None);
        REQUIRE(alloc_result.value == initial_index);
        REQUIRE(sm.GetNewPageOffsetIndex() == initial_index + 1);
    }

    SECTION("Can Write and Read a perfectly aligned page") {
        Byte* write_buffer = AllocateAlignedBuffer();
        Byte* read_buffer = AllocateAlignedBuffer();

        std::memset(write_buffer, 'X', PAGE_SIZE);

        Result<bool> write_res = sm.WritePage(1, write_buffer);
        REQUIRE(write_res.err == ErrType::None);
        REQUIRE(write_res.value == true);

        Result<bool> read_res = sm.ReadPage(1, read_buffer);
        REQUIRE(read_res.err == ErrType::None);
        REQUIRE(read_res.value == true);

        REQUIRE(std::memcmp(write_buffer, read_buffer, PAGE_SIZE) == 0);

        free(write_buffer);
        free(read_buffer);
    }

    SECTION("Persistence: Metadata survives engine restarts") {
        sm.AllocateNewPage();
        sm.AllocateNewPage();
        uint16_t final_index = sm.GetNewPageOffsetIndex();

        {
            StorageManager temp_sm;
        }

        StorageManager reborn_sm;
        REQUIRE(reborn_sm.Bootstrap() == true);

        REQUIRE(reborn_sm.GetNewPageOffsetIndex() == final_index);
    }
}



