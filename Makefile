# Compiler setup
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Target executable name
TARGET = fs_explorer

# Source files and objects
SRCS = BTree.cpp FileSystem.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Compile target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
%.o: %.cpp FileNode.h BTree.h FileSystem.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleanup target files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
