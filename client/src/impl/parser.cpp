
#include "../include/parser.h"

Parser::Parser(std::string &query) {
  current_state = ParserState::EXPECT_COMMAND;
  this->query = query;
  iter = 0;
  this->query_object.valid = false;
};

void Parser::ParseQuery() {
  while(iter < query.size()) {
    char c = query[iter];
    switch (current_state) {
      case ParserState::EXPECT_COMMAND:
        if (c == ' ' || c == '\n') {
          iter++;
        } else if (isalpha(c)) {
          current_state = ParserState::READ_COMMAND;
        } else {
          return;
        };
        break;
      
      case ParserState::READ_COMMAND:
        if (isalpha(c)) {
          temp_buffer.push_back(std::tolower(c));
          iter++;
        } else if (c == ' ') {
          if (temp_buffer == "search") {
            query_object.command = Command::Search;
          }
          else if (temp_buffer == "insert") {
            query_object.command = Command::Insert;
          }
          else if (temp_buffer == "delete") {
            query_object.command = Command::Delete;
          }
          else if (temp_buffer == "quit") {
            query_object.command = Command::Quit;
          }
          else {
            return;
          };
          temp_buffer.clear();
          current_state = ParserState::EXPECT_KEY;
          iter++;
        } else {
          return;
        };
        break;

      case ParserState::EXPECT_KEY:
        if (c == ' ') {
          iter++;
        } else if (isdigit(c)) {
          current_state = ParserState::READ_KEY;
          continue;
        }
        else {
          return;
        };
        break;

      case ParserState::READ_KEY:
        if (isdigit(c)) {
          temp_buffer.push_back(c);
          iter++;
        }
        else if (c == ' ') {
          try {
            Key key = std::stoull(temp_buffer);            
            query_object.key = key;
            current_state = ParserState::EXPECT_PAYLOAD;
            iter++;
            temp_buffer.clear();
          } catch (const std::exception &err) {
            return;
          };
        }
        else if (c == ';') {
          try {
            Key key = std::stoull(temp_buffer);            
            query_object.key = key;
            query_object.valid = true;
            return;
          } catch (const std::exception &err) {
            return;
          };
        }
        else {
          return;
        };
        break;

      case ParserState::EXPECT_PAYLOAD:
        if (c == ' ') {
          iter++;
        }
        else if (c == '"') {
          current_state = ParserState::READ_PAYLOAD;
          iter++;
          continue;
        }
        else if (c == ';') {
          query_object.valid = true;
          return;
        }
        else {
          return;
        };
        break;

      case ParserState::READ_PAYLOAD:
        if (c == '\\') {
          iter++;
          if (iter < query.size()) {
            temp_buffer.push_back(query[iter]);
            iter++;
          } else {
            return;
          };
        }
        else if (c == '"') {
          query_object.payload.push_back(temp_buffer);
          temp_buffer.clear();
          iter++;
          current_state = ParserState::EXPECT_PAYLOAD;
        }
        else {
          temp_buffer.push_back(c);
          iter++;
        };
        break;

    };
  };
};
