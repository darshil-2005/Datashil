#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cctype>
#include <iostream>
#include "../../../commons/types.h"

enum class ParserState : uint8_t {
  EXPECT_COMMAND,
  READ_COMMAND,

  EXPECT_KEY,
  READ_KEY,

  EXPECT_PAYLOAD,
  READ_PAYLOAD,
};

class Parser {
  private:
    ParserState current_state;
    std::string query;
    void ConsumeNextByte();
    size_t iter;
    std::string temp_buffer;

  public:
    Query query_object;
    // Right now keeping things simple and parsing the whole thing in the constructor.
    Parser(std::string &query);
    void ParseQuery();
};
