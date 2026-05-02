#pragma once
#include <math.h>
#include "typedefs.h"

/*********************************************************************/
//
// VNA_MATH.H
//
// Mathematical routines for Vector Network Analyzer (VNA) calibration
// and error correction.
//
/*********************************************************************/

namespace VNA_MATH
{
    // ---------------------------------------------------------------------------
    // 1-Port (3-Term) Error Correction
    // ---------------------------------------------------------------------------
    //
    // Model: S11m = e00 + (e10e01 * S11a) / (1 - e11 * S11a)
    //
    // e00: Directivity
    // e11: Source Match
    // e10e01: Reflection Tracking
    // ---------------------------------------------------------------------------

    bool calculate_1port_error_terms(
        const COMPLEX_DOUBLE& m_open,  // Measured Open
        const COMPLEX_DOUBLE& m_short, // Measured Short
        const COMPLEX_DOUBLE& m_load,  // Measured Load
        const COMPLEX_DOUBLE& a_open,  // Actual/Ideal Open
        const COMPLEX_DOUBLE& a_short, // Actual/Ideal Short
        const COMPLEX_DOUBLE& a_load,  // Actual/Ideal Load
        COMPLEX_DOUBLE& e00,           // OUT: Directivity
        COMPLEX_DOUBLE& e11,           // OUT: Source Match
        COMPLEX_DOUBLE& e10e01         // OUT: Reflection Tracking
    );

    COMPLEX_DOUBLE correct_1port(
        const COMPLEX_DOUBLE& m_data,  // Measured S11
        const COMPLEX_DOUBLE& e00,     // Directivity
        const COMPLEX_DOUBLE& e11,     // Source Match
        const COMPLEX_DOUBLE& e10e01   // Reflection Tracking
    );

    // ---------------------------------------------------------------------------
    // T-Check (Accuracy Check)
    // ---------------------------------------------------------------------------
    // Extracted from Rohde & Schwarz Application Note 1EZ43
    // ---------------------------------------------------------------------------

    DOUBLE calculate_tcheck(
        const COMPLEX_DOUBLE& s11,
        const COMPLEX_DOUBLE& s12,
        const COMPLEX_DOUBLE& s21,
        const COMPLEX_DOUBLE& s22
    );
}
