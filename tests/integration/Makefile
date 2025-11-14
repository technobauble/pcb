# Integration test for C++ action system
# Links C code with C++ actions to validate the bridge works

CC = gcc
CXX = g++
GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
CFLAGS = -std=c99 -Wall -Wextra -I../.. -I../../src -I../../src/actions \
	$(GLIB_CFLAGS) -DCOORD_TYPE=int32_t -DHAVE_STDINT_H
CXXFLAGS = -std=c++11 -Wall -Wextra -I../.. -I../../src -I../../src/actions \
	$(GLIB_CFLAGS) -DCOORD_TYPE=int32_t -DHAVE_STDINT_H
LDFLAGS =

# Test binary
TARGET = action_integration_test

# C sources
C_SOURCES = \
	action_integration_test.c \
	mocks.c

# C++ sources (the action infrastructure)
CXX_SOURCES = \
	../../src/actions/Action.cpp \
	../../src/actions/action_bridge.cpp \
	../../src/actions/MessageAction.cpp \
	../../src/actions/SaveSettingsAction.cpp

C_OBJECTS = $(C_SOURCES:.c=.o)
CXX_OBJECTS = $(CXX_SOURCES:.cpp=.o)
ALL_OBJECTS = $(C_OBJECTS) $(CXX_OBJECTS)

# Build test binary
all: $(TARGET)

# Link with C++ linker (since we have C++ objects)
$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(ALL_OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "===================================="
	@echo "✓ Integration test binary built!"
	@echo "✓ C test code linked with C++ actions"
	@echo "===================================="

# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ sources
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
test: $(TARGET)
	@echo ""
	./$(TARGET)
	@echo ""

# Clean
clean:
	rm -f $(C_OBJECTS) $(CXX_OBJECTS) $(TARGET)
	@echo "Cleaned"

.PHONY: all test clean
