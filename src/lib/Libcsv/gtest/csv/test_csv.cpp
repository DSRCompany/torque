#include <stdio.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "csv.h"

#include "mock.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

const char *test_str="1,3,4";

class cvs_mock {
public:
	cvs_mock() {}
	virtual ~cvs_mock() {}
	int csv_length(const char *csv_str) {return ::csv_length(csv_str);}
};

class gmock_cvs_mock : public cvs_mock {
public:
	MOCK_METHOD1(csv_length, int(const char *csv_str));
};

TEST(Test1, Test2) {
  cvs_mock c;
  gmock_cvs_mock gc;  
  
  EXPECT_CALL(gc, csv_length(test_str));

  EXPECT_EQ(0, gc.csv_length(test_str));
  EXPECT_EQ(1, c.csv_length(test_str));
}
 
int main(int argc, char **argv)
{
  testing::InitGoogleMock(&argc, argv);
  //InitGoogleTest(&argc, argv);
  printf("Hello, world!\n");
 
  return RUN_ALL_TESTS();
}
