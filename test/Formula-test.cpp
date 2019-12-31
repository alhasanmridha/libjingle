#include "gtest/gtest.h"
#include "Formula.h"

TEST(sqTest, test1) {
    EXPECT_EQ (Formula::sq (0),  0);
    EXPECT_EQ (Formula::sq (10), 100);
    EXPECT_EQ (Formula::sq (50), 2500);
}
