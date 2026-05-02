#include <gtest/gtest.h>
#include "pn_math.h"
#include <math.h>

TEST(PNMath, HzString) {
    C8 buf[256];
    EXPECT_STREQ("1.0000 GHz", Hz_string(1E9, buf, 256));
    EXPECT_STREQ("1.0000 MHz", Hz_string(1E6, buf, 256));
    EXPECT_STREQ("1.0000 kHz", Hz_string(1E3, buf, 256));
    EXPECT_STREQ("100.00 Hz", Hz_string(100, buf, 256));
}

TEST(PNMUtil, LogHzString) {
    C8 buf[256];
    EXPECT_STREQ("1000", log_Hz_string(1000, buf, 256));
    EXPECT_STREQ("1000", log_Hz_string(1234, buf, 256));
}

TEST(PNMUtil, RMSNoise) {
    SINGLE freq[] = { 1000.0f, 2000.0f, 3000.0f };
    DOUBLE dbc[]  = { -100.0, -100.0, -100.0 };
    S32 valid[]   = { 1, 1, 1 };
    
    DOUBLE rms, cnr, fm, jitter;
    PN_calculate_rms_noise(freq, dbc, valid, 3, 0, 2, 10E6, rms, cnr, fm, jitter);
    
    EXPECT_GT(rms, 0.0);
    EXPECT_GT(fm, 0.0);
    EXPECT_GT(jitter, 0.0);
    EXPECT_NEAR(cnr, -66.9897, 0.1); // 10*log10(2 * 10^-10 * 1000) = -66.9...
}

TEST(PNMUtil, Math) {
    EXPECT_EQ(10, round_to_nearest(9.6));
    EXPECT_EQ(9, round_to_nearest(9.4));
    EXPECT_NEAR(1.23, round_to_nearest_double(1.2345, 2), 0.001);
    EXPECT_TRUE(epsilon_match(1.0, 1.001));
    EXPECT_FALSE(epsilon_match(1.0, 1.1));
}

TEST(PNMath, SmoothTrace) {
    DOUBLE VW[] = { 1E-10, 1E-10, 1E-8, 1E-10, 1E-10 }; // Log: -100, -100, -80, -100, -100
    S32    VV[] = { 1, 1, 1, 1, 1 };
    DOUBLE smoothed[5] = { 0 };

    PN_smooth_trace(VW, VV, 5, 1, false, smoothed); // Window size 1 (i.e. +/- 1, so 3 samples)

    // Center point should be smoothed: average of (1E-10, 1E-8, 1E-10) = 1.02E-8 / 3 = 3.4E-9
    // 10 * log10(3.4E-9) ~= -84.685 dB
    EXPECT_NEAR(smoothed[2], -84.685, 0.1);

    // Left edge (only 2 samples: 1E-10, 1E-10) -> avg 1E-10 -> -100 dB
    EXPECT_NEAR(smoothed[0], -100.0, 0.1);
}

TEST(PNMath, ClipSpurs) {
    DOUBLE VD[50];
    S32    VV[50];
    
    for (int i=0; i < 50; i++) {
        VD[i] = -100.0;
        VV[i] = 1;
    }
    
    VD[25] = -70.0; // Spike in the middle

    PN_clip_spurs(VD, VV, 50, 20); // 20 dB threshold

    EXPECT_LT(VD[25], -90.0);
    EXPECT_NEAR(VD[24], -100.0, 0.1); 
    EXPECT_NEAR(VD[26], -100.0, 0.1); 
}
