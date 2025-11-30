CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -Iinclude
LDFLAGS = -pthread -lncurses

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include

# List of source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/traffic_sim

.PHONY: all clean directories

all: directories $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) logs

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
