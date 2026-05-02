#ifndef PN_MATH_H
#define PN_MATH_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Math utilities
S32    round_to_nearest(DOUBLE f);
DOUBLE round_to_nearest_double(DOUBLE val, S32 places);
bool   epsilon_match(DOUBLE a, DOUBLE b);

// Frequency string utilities
C8 *Hz_string(DOUBLE val, C8 *dest, S32 dest_size);
C8 *log_Hz_string(DOUBLE Hz, C8 *dest, S32 dest_size);

// Phase Noise specific calculations
void PN_calculate_rms_noise(
    const SINGLE *frequency,    // Array of offset frequencies in Hz
    const DOUBLE *dBc_Hz,       // Array of noise values in dBc/Hz
    const S32    *valid,        // Array of validity flags (non-zero if valid)
    S32           n_points,     // Total number of points in arrays
    S32           L_column,     // Index of lower integration limit
    S32           U_column,     // Index of upper integration limit
    DOUBLE        carrier_Hz,   // Carrier frequency in Hz
    DOUBLE       &RMS_rads,     // Output: RMS phase noise in radians
    DOUBLE       &CNR,          // Output: Carrier-to-Noise Ratio in dB
    DOUBLE       &resid_FM,     // Output: Residual FM in Hz
    DOUBLE       &jitter_s      // Output: Jitter in seconds
);

// Trace Smoothing
void PN_smooth_trace(
    const DOUBLE *VW,             // Array of linear noise values in watts (length: width)
    const S32    *VV,             // Array of validity flags (length: width)
    S32           width,          // Number of screen columns (array length)
    S32           smooth_samples, // Number of samples to smooth
    bool          alt_smoothing,  // Use alternative symmetric smoothing
    DOUBLE       *smoothed_dBc    // Output array (length: width)
);

// Spur Removal
void PN_clip_spurs(
    DOUBLE    *VD,       // Array of noise values in dB (length: width). Will be modified in place.
    const S32 *VV,       // Array of validity flags (length: width)
    S32        width,    // Number of screen columns (array length)
    S32        spur_dB   // Clipping threshold in dB
);

#ifdef __cplusplus
}
#endif

#endif // PN_MATH_H
