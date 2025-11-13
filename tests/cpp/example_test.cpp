/*
 * Example C++ test using Google Test
 *
 * This demonstrates the basic structure of C++ tests for PCB.
 * This is a proof-of-concept and will be expanded as modules migrate.
 */

#include <gtest/gtest.h>

// Example: Testing a simple C++ function
namespace pcb {
namespace test {

// Simple function to demonstrate testing
int add(int a, int b) {
  return a + b;
}

// Basic test
TEST(ExampleTest, SimpleAddition) {
  EXPECT_EQ(5, add(2, 3));
  EXPECT_EQ(0, add(-1, 1));
  EXPECT_EQ(-5, add(-2, -3));
}

// Test with multiple assertions
TEST(ExampleTest, BoundaryConditions) {
  EXPECT_EQ(0, add(0, 0));
  EXPECT_EQ(100, add(100, 0));
  EXPECT_EQ(-100, add(0, -100));
}

// Fixture example
class MathFixture : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup before each test
    value_ = 10;
  }

  void TearDown() override {
    // Cleanup after each test
  }

  int value_;
};

TEST_F(MathFixture, AddWithFixture) {
  EXPECT_EQ(15, add(value_, 5));
}

TEST_F(MathFixture, MultipleTests) {
  EXPECT_EQ(20, add(value_, 10));
  EXPECT_EQ(0, add(value_, -10));
}

} // namespace test
} // namespace pcb
