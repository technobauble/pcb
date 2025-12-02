## -*- makefile -*-
##
## Makefile.am for C++ unit tests using Google Test
##

# C++ test binary
check_PROGRAMS = unittest_cpp

# Unit test sources (standalone tests with no PCB dependencies)
unittest_cpp_SOURCES = \
	example_test.cpp \
	modes/attached_state_test.cpp

# NOTE: mode_manager_test.cpp is an integration test that requires linking
# against the full PCB library. It's excluded from regular 'make check' but
# can be built manually after a full PCB build using:
#   cd tests/cpp && make integration_test

# Include paths
unittest_cpp_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(GTEST_CFLAGS)

# C++ compiler flags
unittest_cpp_CXXFLAGS = \
	-std=c++17 \
	-Wall \
	-Wextra \
	$(GTEST_CXXFLAGS)

# Linker flags and libraries
unittest_cpp_LDADD = \
	$(GTEST_LIBS) \
	-lgtest_main \
	-lpthread

# Coverage support
if ENABLE_COVERAGE
unittest_cpp_CXXFLAGS += $(COVERAGE_CFLAGS)
unittest_cpp_LDFLAGS = $(COVERAGE_LDFLAGS)
endif

# Run tests with 'make check'
TESTS = unittest_cpp

# Additional files to distribute
EXTRA_DIST = \
	README.md \
	pcb_wrapper_example.hpp \
	Makefile.stub \
	modes/mode_manager_test.cpp \
	pcb_stubs.cpp

# Clean generated files
CLEANFILES = \
	*.gcda \
	*.gcno \
	*.gcov
