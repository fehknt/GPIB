#include <gtest/gtest.h>
#include "vna_math.h"
#include <math.h>

class VnaMathTest : public ::testing::Test {
protected:
    const DOUBLE epsilon = 1e-10;
};

TEST_F(VnaMathTest, Ideal1PortCalibration) {
    COMPLEX_DOUBLE m_open(1.0, 0.0);
    COMPLEX_DOUBLE m_short(-1.0, 0.0);
    COMPLEX_DOUBLE m_load(0.0, 0.0);

    COMPLEX_DOUBLE a_open(1.0, 0.0);
    COMPLEX_DOUBLE a_short(-1.0, 0.0);
    COMPLEX_DOUBLE a_load(0.0, 0.0);

    COMPLEX_DOUBLE e00, e11, e10e01;
    ASSERT_TRUE(VNA_MATH::calculate_1port_error_terms(m_open, m_short, m_load, a_open, a_short, a_load, e00, e11, e10e01));

    EXPECT_NEAR(e00.real, 0.0, epsilon);
    EXPECT_NEAR(e00.imag, 0.0, epsilon);
    EXPECT_NEAR(e11.real, 0.0, epsilon);
    EXPECT_NEAR(e11.imag, 0.0, epsilon);
    EXPECT_NEAR(e10e01.real, 1.0, epsilon);
    EXPECT_NEAR(e10e01.imag, 0.0, epsilon);
}

TEST_F(VnaMathTest, OnePortCorrectionRoundTrip) {
    // Artificial error terms
    COMPLEX_DOUBLE e00(0.1, 0.05);      // Directivity
    COMPLEX_DOUBLE e11(0.05, -0.02);    // Source Match
    COMPLEX_DOUBLE e10e01(0.9, 0.1);    // Reflection Tracking

    // Actual DUT
    COMPLEX_DOUBLE actual(0.5, -0.3);

    // S11m = e00 + (e10e01 * S11a) / (1 - e11 * S11a)
    COMPLEX_DOUBLE num = e10e01 * actual;
    COMPLEX_DOUBLE den = COMPLEX_DOUBLE(1.0, 0.0) - e11 * actual;
    COMPLEX_DOUBLE measured = e00 + (num / den);

    // Correct it
    COMPLEX_DOUBLE corrected = VNA_MATH::correct_1port(measured, e00, e11, e10e01);

    EXPECT_NEAR(corrected.real, actual.real, epsilon);
    EXPECT_NEAR(corrected.imag, actual.imag, epsilon);
}

TEST_F(VnaMathTest, TCheckIdeal) {
    // Ideal 2-port through
    COMPLEX_DOUBLE s11(0.0, 0.0);
    COMPLEX_DOUBLE s12(1.0, 0.0);
    COMPLEX_DOUBLE s21(1.0, 0.0);
    COMPLEX_DOUBLE s22(0.0, 0.0);

    DOUBLE tcheck = VNA_MATH::calculate_tcheck(s11, s12, s21, s22);
    EXPECT_NEAR(tcheck, 0.0, epsilon);
}

TEST_F(VnaMathTest, TCheckAssymetry) {
    // Slightly asymmetrical
    COMPLEX_DOUBLE s11(0.01, 0.0);
    COMPLEX_DOUBLE s12(0.95, 0.0);
    COMPLEX_DOUBLE s21(0.94, 0.0);
    COMPLEX_DOUBLE s22(0.01, 0.0);

    DOUBLE tcheck = VNA_MATH::calculate_tcheck(s11, s12, s21, s22);
    // Should be non-zero
    EXPECT_GT(fabs(tcheck), 0.0);
}
