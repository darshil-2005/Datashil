#include "../../include/storageManager/storageManager.h"

namespace fs = std::filesystem;

bool StorageManager::Bootstrap(std::string DB_PATH) {

  std::cout << "[Storage Manager] Bootstraping..." << std::endl;

  fs::path file_path(DB_PATH);
  fs::path DATA_DIR = file_path.parent_path();

  if (!fs::exists(DATA_DIR)) {
    fs::create_directories(DATA_DIR);
    std::cout << "[Storage Manager] Initialized data directory: " << DATA_DIR << std::endl;
  };

  if (!DATA_DIR.empty() && !fs::exists(DB_PATH)) {
    fd_database = open(DB_PATH.c_str(), O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
  } else {
    fd_database = open(DB_PATH.c_str(), O_RDWR | O_DIRECT, S_IRUSR | S_IWUSR);
  };
  
  if (!StorageManager::IsDatabaseFile(fd_database)) {
    Byte buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);
    uint32_t magic_to_write = STORAGE_MANAGER_MAGIC_NUMBER;
    memcpy(buffer, &magic_to_write, MAGIC_NUMBER_SIZE);
    uint64_t off_start = DATABASE_META_PAGE_ID + 1;
    memcpy(buffer + MAGIC_NUMBER_SIZE, &off_start, sizeof(uint64_t));
    StorageManager::PrivateWritePage(STORAGE_MANAGER_META_PAGE_ID, buffer);
    memset(buffer, 0, PAGE_SIZE); 
    StorageManager::PrivateWritePage(DATABASE_META_PAGE_ID, buffer);
  };

  // set page offset from the file
  Byte storage_manager_meta_page[PAGE_SIZE];
  StorageManager::PrivateReadPage(STORAGE_MANAGER_META_PAGE_ID, storage_manager_meta_page);
  memcpy(&new_page_offset_index, storage_manager_meta_page + MAGIC_NUMBER_SIZE, sizeof(uint64_t));

  if (fd_database == -1) {
    return false;
  };

  return true;
};


bool StorageManager::IsDatabaseFile(int file_descriptor) {

  struct stat st;

  if (fstat(fd_database, &st) == -1) {
    return false;
  };
  if (st.st_size < 0 || static_cast<size_t>(st.st_size) < PAGE_SIZE) {
    return false;
  };

  Byte page[PAGE_SIZE];
  Offset off = STORAGE_MANAGER_META_PAGE_ID * PAGE_SIZE;
  pread(file_descriptor, page, PAGE_SIZE, off);
  // handle errors
  uint32_t retrieved_magic_number;
  memcpy(&retrieved_magic_number, page, MAGIC_NUMBER_SIZE);
  if (retrieved_magic_number == STORAGE_MANAGER_MAGIC_NUMBER) {
    return true;
  };
  return false;
};

uint64_t StorageManager::GetNewPageOffsetIndex() {
  return new_page_offset_index;
};

Result<bool> StorageManager::PrivateReadPage(PageID pid, Byte *buffer) {

  Offset off = pid * PAGE_SIZE;
  ssize_t byte_count = pread(fd_database, buffer, PAGE_SIZE, off);

  if (byte_count == -1) {
    return {.value = false, .err = ErrType::SystemErr};
  };

  if (static_cast<size_t>(byte_count) != PAGE_SIZE) {
    return {.value = false, .err = ErrType::FileCorruption};
  }

  return {.value = true, .err = ErrType::None};
};

Result<bool> StorageManager::ReadPage(PageID pid, Byte *buffer) {

  if (pid == STORAGE_MANAGER_META_PAGE_ID) {
    return { .value = false, .err = ErrType::OperationNotAllowed };
  };

  Offset off = pid * PAGE_SIZE;
  ssize_t byte_count = pread(fd_database, buffer, PAGE_SIZE, off);

  if (byte_count == -1) {
    return {.value = false, .err = ErrType::SystemErr};
  };

  if (static_cast<size_t>(byte_count) != PAGE_SIZE) {
    return {.value = false, .err = ErrType::FileCorruption};
  }

  return {.value = true, .err = ErrType::None};
};

Result<bool> StorageManager::PrivateWritePage(PageID pid, const Byte *buffer) {

  Offset off = pid * PAGE_SIZE;

  ssize_t byte_count = pwrite(fd_database, buffer, PAGE_SIZE, off);

  if (byte_count == -1) {
    return {.value = false, .err = ErrType::SystemErr};
  };

  if (static_cast<size_t>(byte_count) != PAGE_SIZE) {
    return {.value = false, .err = ErrType::DiskFullOrTruncated};
  };

  return {.value = true, .err = ErrType::None};
};

Result<bool> StorageManager::WritePage(PageID pid, const Byte *buffer) {

  if (pid == STORAGE_MANAGER_META_PAGE_ID) {
    return { .value = false, .err = ErrType::OperationNotAllowed };
  };

  Offset off = pid * PAGE_SIZE;

  ssize_t byte_count = pwrite(fd_database, buffer, PAGE_SIZE, off);

  if (byte_count == -1) {
    return {.value = false, .err = ErrType::SystemErr};
  };

  if (static_cast<size_t>(byte_count) != PAGE_SIZE) {
    return {.value = false, .err = ErrType::DiskFullOrTruncated};
  };

  return {.value = true, .err = ErrType::None};
};

StorageManager::~StorageManager() {
  std::cout << "[Shutdown] Destructor triggered. Releasing system resources..." << std::endl;

  if (fd_database != -1) {
    if (close(fd_database) == 0) {
      std::cout << "[Shutdown] fd_database (FD: " << fd_database
                << ") closed successfully." << std::endl;
    } else {
      std::cerr << "[Shutdown] Warning: Failed to close fd_database cleanly."
                << std::endl;
    };
    fd_database = -1;
  };
};

void StorageManager::SetNewPageOffsetIndex(uint64_t new_off) {
  Byte page[PAGE_SIZE];
  StorageManager::PrivateReadPage(STORAGE_MANAGER_META_PAGE_ID, page);
  memcpy(page + MAGIC_NUMBER_SIZE, &new_off, sizeof(uint64_t));
  StorageManager::PrivateWritePage(STORAGE_MANAGER_META_PAGE_ID, page);
  return;
};

Result<PageID> StorageManager::AllocateNewPage() {

  Offset offset = new_page_offset_index * PAGE_SIZE;
  int alloc_result = posix_fallocate(fd_database, offset, PAGE_SIZE);

  if (alloc_result != 0) {
    // handle errors
  };

  StorageManager::SetNewPageOffsetIndex(new_page_offset_index + 1);
  new_page_offset_index++;
  return { .value = (PageID)(new_page_offset_index - 1), .err = ErrType::None };  
};
