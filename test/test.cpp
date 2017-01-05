//=============================================================================
//
// Name:        test.cpp
// Authors:     James H. Loving
// Description: This file defines all unit tests for edict.
//
//=============================================================================

#include "gtest/gtest.h"

TEST(TestHierarchy, TestTest)
{
    EXPECT_EQ(1, 1);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
