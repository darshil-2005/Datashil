
BUILD_DIR = build
CXX = g++
CXXFLAGS = -std=c++17 -Wall

UTILS_PATH = commons/src/impl/utils/utils.cpp

SERVER_BIN = server_app

SERVER_SRCS = database/src/main.cpp \
							$(wildcard database/src/impl/*/*.cpp) \
							$(UTILS_PATH)

SERVER_OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SERVER_SRCS))

CLIENT_BIN = client_app

CLIENT_SRCS = client/src/main.cpp \
							$(wildcard client/src/impl/*.cpp) \
							$(UTILS_PATH)

CLIENT_OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(CLIENT_SRCS))

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN) : $(SERVER_OBJS)
	@echo "Linking Server..."
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT_BIN) : $(CLIENT_OBJS)
	@echo "Linking Client..."
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Nuking build directory..."
	rm -rf $(BUILD_DIR) $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean
