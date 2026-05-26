# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O3

# Default build = debug
BUILD_FLAGS = $(CXXFLAGS) $(DEBUG_FLAGS)

# If RELEASE=1 → optimized, no debug symbols
ifeq ($(RELEASE),1)
    BUILD_FLAGS = $(CXXFLAGS) $(RELEASE_FLAGS)
endif

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = makeBuild

# Files
SRC = $(shell find $(SRC_DIR) -name "*.cpp")
HDR = $(shell find $(INC_DIR) -name "*.hpp")
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC))

# Linker flags
LDFLAGS = -lssl -lcrypto -lz

# Target binary
TARGET = twig

# Ensure build directory exists
$(shell mkdir -p $(BUILD_DIR))

# ── targets ───────────────────────────────────────────────

$(TARGET): $(OBJ)
	$(CXX) $(BUILD_FLAGS) -I $(INC_DIR) -o $(TARGET) $(OBJ) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(BUILD_FLAGS) -I $(INC_DIR) -c $< -o $@

release:
	$(MAKE) RELEASE=1

rebuild: clean $(TARGET)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

.PHONY: clean release rebuild