#include "sparams.h"
#include <gtest/gtest.h>
#include <fstream>

TEST(SParams, Basic) {
    SPARAMS s;
    
    // Test conversion between formats
    SPARAM::MA ma(1.0, 45.0);
    SPARAM::DB db(ma); 
    
    ASSERT_NEAR(db.dB, 0.0, 0.0001);
    ASSERT_NEAR(db.deg, 45.0, 0.0001);
    
    SPARAM::RI ri(db);
    ASSERT_NEAR(ri.real, cos(45.0 * 3.14159265358979 / 180.0), 0.0001);
    ASSERT_NEAR(ri.imag, sin(45.0 * 3.14159265358979 / 180.0), 0.0001);
}

TEST(SParams, Interpolation) {
    SPARAMS s;
    s.alloc(1, 2);
    s.freq_Hz[0] = 100.0;
    s.freq_Hz[1] = 200.0;
    s.min_Hz = 100.0;
    s.max_Hz = 200.0;
    s.set_RI(0, 0, 0, COMPLEX_DOUBLE(1.0, 0.0));
    s.set_RI(1, 0, 0, COMPLEX_DOUBLE(2.0, 0.0));
    
    bool in_range = false;
    SPARAM::RI res = s.get_RI(150.0, 0, 0, 0, &in_range);
    EXPECT_TRUE(in_range);
    EXPECT_NEAR(res.real, 1.5, 0.0001);
    EXPECT_NEAR(res.imag, 0.0, 0.0001);
    
    // Test extrapolation
    // Note: in_range stays TRUE if extrapolation flag is used, because a "valid" value was produced.
    res = s.get_RI(250.0, 0, 0, SPARAM::EXT_REND, &in_range);
    EXPECT_TRUE(in_range); 
    EXPECT_NEAR(res.real, 2.0, 0.0001);
}

TEST(SParams, FileIO) {
    const char* filename = "test_io.s1p";
    {
        SPARAMS s;
        s.alloc(1, 2);
        s.freq_Hz[0] = 1e9;
        s.freq_Hz[1] = 2e9;
        s.min_Hz = 1e9;
        s.max_Hz = 2e9;
        s.set_RI(0, 0, 0, COMPLEX_DOUBLE(0.707, 0.707));
        s.set_RI(1, 0, 0, COMPLEX_DOUBLE(1.0, 0.0));
        ASSERT_TRUE(s.write_SNP_file(filename, "RI", "GHZ"));
    }
    
    {
        SPARAMS s;
        ASSERT_TRUE(s.read_SNP_file(filename, 1));
        EXPECT_EQ(s.n_points, 2);
        EXPECT_NEAR(s.freq_Hz[0], 1e9, 1.0);
        SPARAM::RI ri = s.get_RI(0, 0, 0);
        EXPECT_NEAR(ri.real, 0.707, 0.001);
    }
    remove(filename);
}
