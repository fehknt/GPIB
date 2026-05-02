#include "spline.h"
#include <gtest/gtest.h>

TEST(Spline, Basic) {
    DOUBLE x1[] = { 0.0, 1.0, 2.0, 3.0 };
    DOUBLE y1[] = { 0.0, 1.0, 0.5, 0.0 };
    S32 len1 = 4;
    
    DOUBLE x2[] = { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 };
    DOUBLE y2[7] = { 0 };
    S32 len2 = 7;
    
    // Test spline_gen
    spline_gen(x1, y1, len1, x2, y2, len2);
    
    ASSERT_NEAR(y2[0], 0.0, 0.0001);
    ASSERT_NEAR(y2[2], 1.0, 0.0001);
    ASSERT_NEAR(y2[4], 0.5, 0.0001);
    ASSERT_NEAR(y2[6], 0.0, 0.0001);
}

TEST(Spline, TwoPoints) {
    DOUBLE x1[] = { 0.0, 10.0 };
    DOUBLE y1[] = { 0.0, 10.0 };
    S32 len1 = 2;
    
    DOUBLE x2[] = { 0.0, 5.0, 10.0 };
    DOUBLE y2[3] = { 0 };
    S32 len2 = 3;
    
    // 2-point spline should be linear
    spline_gen(x1, y1, len1, x2, y2, len2);
    
    EXPECT_NEAR(y2[0], 0.0, 0.0001);
    EXPECT_NEAR(y2[1], 5.0, 0.0001);
    EXPECT_NEAR(y2[2], 10.0, 0.0001);
}

TEST(Spline, Lerp) {
    DOUBLE x1[] = { 0.0, 1.0, 2.0 };
    DOUBLE y1[] = { 0.0, 10.0, 5.0 };
    S32 len1 = 3;
    
    DOUBLE x2[] = { 0.0, 0.5, 1.0, 1.5, 2.0 };
    DOUBLE y2[5] = { 0 };
    S32 len2 = 5;
    
    lerp_gen(x1, y1, len1, x2, y2, len2);
    
    EXPECT_NEAR(y2[1], 5.0, 0.0001);
    EXPECT_NEAR(y2[3], 7.5, 0.0001);
}
