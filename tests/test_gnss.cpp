#include <gtest/gtest.h>
#include "gnss_utils.h"
#include "timeutil.h"

TEST(GnssUtils, CoordConversion) {
    // 3015.50 N -> 30.258333
    EXPECT_NEAR(ddmm2degrees(3015.50, 'N'), 30.258333, 0.00001);
    // 09745.25 W -> -97.754167
    EXPECT_NEAR(ddmm2degrees(9745.25, 'W'), -97.754167, 0.00001);
    
    bool neg = false;
    EXPECT_NEAR(degrees2ddmm(30.258333, &neg), 3015.50, 0.01);
    EXPECT_FALSE(neg);
    
    EXPECT_NEAR(degrees2ddmm(-97.754167, &neg), 9745.25, 0.01);
    EXPECT_TRUE(neg);
}

TEST(GnssUtils, Checksum) {
    EXPECT_TRUE(verify_nmea_checksum("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47"));
    EXPECT_FALSE(verify_nmea_checksum("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48"));
}

TEST(GnssUtils, ParseGPGGA) {
    GpggaData data;
    const char* line = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
    ASSERT_TRUE(parse_gpgga(line, data));
    
    EXPECT_NEAR(data.UTC, 123519.0, 0.1);
    EXPECT_NEAR(data.latitude, 4807.038, 0.001);
    EXPECT_EQ(data.lat_sign, 'N');
    EXPECT_EQ(data.fix_quality, 1);
    EXPECT_EQ(data.n_SV, 8);
}

TEST(GnssUtils, ParseGPRMC) {
    GprmcData data;
    const char* line = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
    ASSERT_TRUE(parse_gprmc(line, data));
    
    EXPECT_NEAR(data.UTC, 123519.0, 0.1);
    EXPECT_EQ(data.status, 'A');
    EXPECT_NEAR(data.latitude, 4807.038, 0.001);
    EXPECT_EQ(data.lat_sign, 'N');
    EXPECT_NEAR(data.longitude, 1131.000, 0.001);
    EXPECT_EQ(data.long_sign, 'E');
    EXPECT_NEAR(data.speed, 22.4, 0.1);
    EXPECT_NEAR(data.track, 84.4, 0.1);
    EXPECT_EQ(data.date, 230394);
}

TEST(GnssUtils, TimeSync) {
    // 23-Mar-1994 12:35:19 UTC
    S64 ts = nmea_to_us(230394, 123519.0);
    
    C8 text[128];
    USTIMER::timestamp(text, sizeof(text), ts);
    
    // We expect the timestamp to contain "1994" and "Mar" or "03" and "23"
    // Note: timestamp format depends on locale, but let's check for year at least
    EXPECT_STRNE(strstr(text, "1994"), NULL);
}
