#include "vna_math.h"
#include <math.h>

namespace VNA_MATH
{
    // ---------------------------------------------------------------------------
    // 1-Port (3-Term) Error Correction
    // ---------------------------------------------------------------------------

    bool calculate_1port_error_terms(
        const COMPLEX_DOUBLE& m_open,
        const COMPLEX_DOUBLE& m_short,
        const COMPLEX_DOUBLE& m_load,
        const COMPLEX_DOUBLE& a_open,
        const COMPLEX_DOUBLE& a_short,
        const COMPLEX_DOUBLE& a_load,
        COMPLEX_DOUBLE& e00,
        COMPLEX_DOUBLE& e11,
        COMPLEX_DOUBLE& e10e01
    )
    {
        // Solving the system of 3 linear equations for e00, e11, e10e01:
        // S_m * (1 - S_a * e11) = e00 * (1 - S_a * e11) + e10e01 * S_a
        // S_m - S_m * S_a * e11 = e00 - e00 * S_a * e11 + e10e01 * S_a
        // S_m = e00 + S_a * S_m * e11 - S_a * e00 * e11 + S_a * e10e01
        // S_m = e00 + S_a * (S_m * e11 - e00 * e11 + e10e01)
        // S_m = e00 + S_a * (S_m * e11 + Delta_e) where Delta_e = e10e01 - e00 * e11
        
        // System in terms of e00, e11, Delta_e:
        // m_o = e00 + a_o * m_o * e11 + a_o * Delta_e
        // m_s = e00 + a_s * m_s * e11 + a_s * Delta_e
        // m_l = e00 + a_l * m_l * e11 + a_l * Delta_e

        // [ 1  a_o*m_o  a_o ] [ e00     ]   [ m_o ]
        // [ 1  a_s*m_s  a_s ] [ e11     ] = [ m_s ]
        // [ 1  a_l*m_l  a_l ] [ Delta_e ]   [ m_l ]

        COMPLEX_DOUBLE A11 = 1.0, A12 = a_open * m_open,  A13 = a_open;
        COMPLEX_DOUBLE A21 = 1.0, A22 = a_short * m_short, A23 = a_short;
        COMPLEX_DOUBLE A31 = 1.0, A32 = a_load * m_load,   A33 = a_load;

        COMPLEX_DOUBLE det = A11 * (A22 * A33 - A23 * A32) -
                            A12 * (A21 * A33 - A23 * A31) +
                            A13 * (A21 * A32 - A22 * A31);

        if (det.cabs() < 1e-15)
        {
            return false;
        }

        e00 = (m_open  * (A22 * A33 - A23 * A32) -
               A12     * (m_short * A33 - A23 * m_load) +
               A13     * (m_short * A32 - A22 * m_load)) / det;

        e11 = (A11     * (m_short * A33 - A23 * m_load) -
               m_open  * (A21 * A33 - A23 * A31) +
               A13     * (A21 * m_load - m_short * A31)) / det;

        COMPLEX_DOUBLE Delta_e = (A11 * (A22 * m_load - m_short * A32) -
                                 A12 * (A21 * m_load - m_short * A31) +
                                 m_open * (A21 * A32 - A22 * A31)) / det;

        e10e01 = Delta_e + e00 * e11;

        return true;
    }

    COMPLEX_DOUBLE correct_1port(
        const COMPLEX_DOUBLE& m_data,
        const COMPLEX_DOUBLE& e00,
        const COMPLEX_DOUBLE& e11,
        const COMPLEX_DOUBLE& e10e01
    )
    {
        // S11a = (S11m - e00) / (e10e01 + e11 * (S11m - e00))
        COMPLEX_DOUBLE num = m_data - e00;
        COMPLEX_DOUBLE den = e10e01 + e11 * num;

        if (den.cabs() < 1e-15)
        {
            return m_data; // Avoid division by zero
        }

        return num / den;
    }

    DOUBLE calculate_tcheck(
        const COMPLEX_DOUBLE& s11,
        const COMPLEX_DOUBLE& s12,
        const COMPLEX_DOUBLE& s21,
        const COMPLEX_DOUBLE& s22
    )
    {
        DOUBLE b11 = s11.cabs();
        DOUBLE b12 = s12.cabs();
        DOUBLE c11 = s21.cabs();
        DOUBLE c12 = s22.cabs();

        DOUBLE den = COMPLEX_DOUBLE::csqrt((1.0 - b11 * b11 - b12 * b12) * (1.0 - c11 * c11 - c12 * c12)).cabs();

        if (fabs(den) < 1E-30)
        {
            return 0.0;
        }

        COMPLEX_DOUBLE num = ((s11 * s21.conj()) + (s12 * s22.conj()));

        return ((num.cabs() / den) - 1.0) * 100.0;
    }
}
