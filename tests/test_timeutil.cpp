#include "timeutil.h"
#include <gtest/gtest.h>
#include <windows.h>

TEST(TimeUtil, TimerBasic) {
    USTIMER t;
    
    // Timebase starts at 0 on construction
    Sleep(100);
    S64 elapsed = t.us();
    
    // Sleep is not precise, but should be around 100ms (100,000 us)
    EXPECT_GE(elapsed, 50000LL); 
    EXPECT_LE(elapsed, 500000LL);
}

TEST(TimeUtil, MJDMath) {
    // Nov 17, 1858 should be MJD 0
    EXPECT_NEAR(USTIMER::date_to_MJD(1858, 11, 17), 0.0, 0.0001);
    
    // Jan 1, 1970 should be MJD 40587
    EXPECT_NEAR(USTIMER::date_to_MJD(1970, 1, 1), 40587.0, 0.0001);
    
    // MJD to Windows File Time (us since 1601)
    // Jan 1, 1970 00:00:00 UTC
    S64 ts = USTIMER::MJD_to_us(40587.0);
    
    EXPECT_EQ(ts, SYS_TIME_1970 * 1000000LL);
}

TEST(TimeUtil, FormatStrings) {
    // Test date_text and time_text
    // Use a date that is definitely in the same year globally, e.g. July 1st, 2000
    // Jan 1, 2000 is MJD 51544. July 1st is ~182 days later.
    S64 ts = USTIMER::MJD_to_us(51544.0 + 182.0);
    
    C8 text[128];
    USTIMER::date_text(ts, text, sizeof(text));
    
    // Should contain "2000"
    EXPECT_STRNE(strstr(text, "2000"), NULL);
    
    USTIMER::time_text(ts, text, sizeof(text));
    EXPECT_GT(strlen(text), 0);
}
