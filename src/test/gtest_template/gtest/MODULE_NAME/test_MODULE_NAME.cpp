#include <stdio.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

TEST(Test1, Test2) {
  EXPECT_EQ(0, 0);
}
 
int main(int argc, char **argv)
{
  testing::InitGoogleMock(&argc, argv);
  //InitGoogleTest(&argc, argv);
 
  return RUN_ALL_TESTS();
}
