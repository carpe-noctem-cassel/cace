// Bring in my package's API, which is what I'm testing
//#include "SystemConfig.h"
// Bring in gtest
#include <gtest/gtest.h>
#include <stdio.h>

//using namespace cace;

TEST(SystemConfigBasics, ownRobotID)
{
	EXPECT_TRUE(true);
}


TEST(SystemConfigBasics, blablabla)
{
	EXPECT_TRUE(true);
}


// Run all the tests that were declared with TEST()
int main(int argc, char **argv){
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

