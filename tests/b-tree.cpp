#include "../src/include/b-tree/b-tree.h"
#include "../src/include/bufferpool/bufferpool.h"
#include "../src/include/storageManager/storageManager.h"
#include "./utils/catch.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

struct TestDataset {
  std::vector<Key> keys;
  std::vector<std::string> payloads;
};

TestDataset LoadBulkTestData(const std::string& filepath) {
  TestDataset dataset;
  std::ifstream file(filepath);
  
  if (!file.is_open()) {
    throw std::runtime_error("Could not open test data file.");
  }

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string key_str, payload_str;

    // Parse the CSV (Key,Payload)
    if (std::getline(ss, key_str, ',') && std::getline(ss, payload_str)) {
      dataset.keys.push_back(std::stoull(key_str)); // Assuming Key is an integer type
      dataset.payloads.push_back(payload_str);
    }
  }
  
  return dataset;
};

// Function that takes in a bunch of string and converts them to the form
// [KEY][STRING] in a buffer.
void PrepareBuffer(Byte *buffer, std::vector<Key> &keys,
                   std::vector<std::string> &str_vec) {
  size_t off = 0;
  for (int i = 0; i < str_vec.size(); i++) {
    memcpy(buffer + off, &keys[i], sizeof(Key));
    off += sizeof(Key);
    memcpy(buffer + off, str_vec[i].data(), str_vec[i].size());
    off += str_vec[i].size();
  };
};

TEST_CASE("Insert function properly inserts small and large tuples with strict increasing by 1 order of keys in the BTree.", "[b-tree][write-normal]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  std::cout << "Before Insert: \n";
  bf.DumpCurrBufferPool();

  Byte buffer[100000];

  int num_records = 60000;

  for (int i=0; i<num_records; i++) {
    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i]);
    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    REQUIRE(diff == 0);
  };

  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;
};

TEST_CASE("BTree maintains persistence normal.", "[b-tree][read-normal]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 1336);

  // DumpPage2(bf.RequestPage(25240).value);
  // bf.ReleasePage(25240, false);

  int num_records = 60000;

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);
    
    PayloadStream stream = bt.Search(1336, dataset.keys[i]);
    REQUIRE(stream.curr_pid != 0);

    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    // std::cout << "KEY: " << *reinterpret_cast<uint16_t*>(result) << std::endl;
    // std::cout << "REC STRING: " << recovered << std::endl;

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff != 0) { std::cout << "INDEX: " << i << std::endl; };
    REQUIRE(diff == 0);
  };
  
};

TEST_CASE("Insert function properly inserts small and large tuples with no particular order of keys in the BTree.", "[b-tree][write-jumbled]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_jumbled.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];

  int num_records = 60000;

  for (int i=0; i<num_records; i++) {

    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i]);
    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    REQUIRE(diff == 0);
  };

  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;
  
  // DumpPage2(bf.RequestPage(25240).value);
  // bf.ReleasePage(25240, false);
};

TEST_CASE("BTree maintains persistence jumbled.", "[b-tree][read-jumbled]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_jumbled.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 1442);

  // DumpPage2(bf.RequestPage(25240).value);
  // bf.ReleasePage(25240, false);

  int num_records = 60000;

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);
    
    PayloadStream stream = bt.Search(1442, dataset.keys[i]);
    REQUIRE(stream.curr_pid != 0);

    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    // std::cout << "KEY: " << *reinterpret_cast<uint16_t*>(result) << std::endl;
    // std::cout << "REC STRING: " << recovered << std::endl;

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff != 0) { std::cout << "INDEX: " << i << std::endl; };
    REQUIRE(diff == 0);
  };
};

TEST_CASE("Borrowing from right works", "[b-tree][delete][borrow_right]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_small.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];

  int num_records = 100;

  for (int i=0; i<num_records; i++) {

    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };
  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;

  int nd = 20;

  for (int i=0; i<nd; i++) {
    bt.Delete(bt.root_page_id, dataset.keys[i]);
  };

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i]);
    if (i < nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };
    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };

  std::cout << "First Leaf Page\n";
  LeafPage::DumpPageFirstLast(bf.RequestPage(2).value);
  bf.ReleasePage(2, false);
  std::cout << "\n";

  std::cout << "Final Leaf Page\n";
  LeafPage::DumpPageFirstLast(bf.RequestPage(3).value);
  bf.ReleasePage(3, false);
  std::cout << "\n";
  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;

};

TEST_CASE("BorrowLeft works", "[b-tree][delete][borrow_left]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_small.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];

  int num_records = 100;

  for (int i=0; i<num_records; i++) {

    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };
  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;

  int nd = 20;

  for (int i=num_records-1; i >= num_records - nd; i--) {
    bt.Delete(bt.root_page_id, dataset.keys[i]);
  };

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i]);
    if (i >= num_records - nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };
    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };

  std::cout << "First Leaf Page\n";
  LeafPage::DumpPageFirstLast(bf.RequestPage(2).value);
  bf.ReleasePage(2, false);
  std::cout << "\n";

  std::cout << "Final Leaf Page\n";
  LeafPage::DumpPageFirstLast(bf.RequestPage(3).value);
  bf.ReleasePage(3, false);
  std::cout << "\n";
  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;

};

TEST_CASE("Defragmentation works", "[b-tree][defrag]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_small.csv");
  
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];

  int num_records = 50;

  for (int i=0; i<num_records; i++) {

    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  std::cout << "Root ID: " << bt.root_page_id << "\n";

  int nd = 10;
  for (int i=0; i < nd; i++) {
    bt.Delete(bt.root_page_id, dataset.keys[i]);
  };

  Byte* page = bf.RequestPage(bt.root_page_id).value;
  LeafPage::DefragmentPage(page);
  bf.ReleasePage(bt.root_page_id, true);

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i]);
    if (i<nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };
    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };
};

TEST_CASE("Delete from left works", "[b-tree][delete][merge][delete_left]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load.csv");
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];
  int num_records = 60000;

  std::cout << "Before Insert: " << bf.DumpCurrBufferPool() << std::endl;
  for (int i=0; i<num_records; i++) {
    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  int nd = 30000;

  std::cout << "After Insert: " << bf.DumpCurrBufferPool() << std::endl;

  for (int i=0; i < nd; i++) {
    int b = bf.DumpCurrBufferPool(false);
    bt.Delete(bt.root_page_id, dataset.keys[i]);
    int a = bf.DumpCurrBufferPool(false);
    if (a!=b) {
      std::cout << "Problem Key: " << dataset.keys[i] << std::endl;
    };
  };

  std::cout << "After Delete: " << bf.DumpCurrBufferPool() << std::endl;

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i], false);

    if (i < nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };

    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };

  std::cout << "Root Key: " << bt.root_page_id << std::endl;
};

TEST_CASE("Delete from right works", "[b-tree][delete][merge][delete_right]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_small.csv");
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];
  int num_records = 60000;

  for (int i=0; i<num_records; i++) {
    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  int nd = 30000;

  for (int i=num_records-1; i >= num_records - nd; i--) {
    bt.Delete(bt.root_page_id, dataset.keys[i]);
  };

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i], false);

    if (i >= num_records - nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };

    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };
};

TEST_CASE("Delete works for random deletes.", "[b-tree][delete][merge][delete_random]") {

  TestDataset dataset = LoadBulkTestData("tests/data/btree_test_load_small.csv");
  REQUIRE(dataset.keys.size() == 60000);
  
  if (fs::exists(DATA_DIR)) {
    fs::remove_all(DATA_DIR);
  };
  
  StorageManager sm;
  REQUIRE(sm.Bootstrap() == true);
  BufferPool bf(sm);
  BTree bt(bf, 0);

  Byte buffer[100000];
  int num_records = 1000;

  for (int i=0; i<num_records; i++) {
    memcpy(buffer, &dataset.keys[i], sizeof(Key));
    memcpy(buffer + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());

    REQUIRE(bt.InsertTuple(buffer, dataset.payloads[i].size() + sizeof(Key), dataset.keys[i]) == true);
  };

  int nd = 100;

  for (int i=999; i >= num_records - nd; i--) {
    bt.Delete(bt.root_page_id, dataset.keys[i]);
  };

  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;

  Byte result[100000];
  for (int i=0; i<num_records; i++) {
    size_t n = dataset.payloads[i].size() + sizeof(Key);

    PayloadStream stream = bt.Search(bt.root_page_id, dataset.keys[i], false);

    if (i >= num_records - nd) {
      REQUIRE(stream.curr_pid == 0);
      continue;
    };

    stream.NextBytes(result, n);
    std::string recovered(reinterpret_cast<char*>(result + sizeof(Key)), n - sizeof(Key));

    REQUIRE(recovered.size() == dataset.payloads[i].size());
    int diff = memcmp(result + sizeof(Key), dataset.payloads[i].data(), dataset.payloads[i].size());
    if (diff!=0) {

    };
    REQUIRE(diff == 0);
  };

  std::cout << "ROOT PAGE ID: " << bt.root_page_id << std::endl;
};
