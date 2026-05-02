#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include "pn_math.h"

// *****************************************************************************
// Math utilities
// *****************************************************************************

S32 round_to_nearest(DOUBLE f)
{
   if (f < 0.0)
      return S32(f - 0.5);
   else
      return S32(f + 0.5);
}

DOUBLE round_to_nearest_double(DOUBLE val, S32 places)
{
   DOUBLE sd = pow(10.0, (DOUBLE)places);

   if (val < 0.0)
      return ceil((val * sd) - 0.5) / sd;
   else
      return floor((val * sd) + 0.5) / sd;
}

bool epsilon_match(DOUBLE a, DOUBLE b)
{
   DOUBLE absa = fabs(a);
   DOUBLE absb = fabs(b);

   DOUBLE epsilon = (std::min)(absa, absb) / 100.0;

   if (epsilon < 1E-9) epsilon = 1E-9;

   return (fabs(a - b) < epsilon);
}

// *****************************************************************************
// Frequency string utilities
// *****************************************************************************

C8 *Hz_string(DOUBLE val, C8 *dest, S32 dest_size)
{
   DOUBLE v = fabs(val);

   if (v >= 1E9)
      _snprintf(dest, dest_size - 1, "%.04lf GHz", val / 1E9);
   else if (v >= 1E6)
      _snprintf(dest, dest_size - 1, "%.04lf MHz", val / 1E6);
   else if (v >= 1E3)
      _snprintf(dest, dest_size - 1, "%.04lf kHz", val / 1E3);
   else
      _snprintf(dest, dest_size - 1, "%.02lf Hz", val);

   return dest;
}

C8 *log_Hz_string(DOUBLE Hz, C8 *dest, S32 dest_size)
{
   memset(dest, 0, dest_size);

   if (Hz == 0.0)
   {
      strcpy(dest, "0");
      return dest;
   }

   S32 dec = (S32)floor(log10(Hz));

   if (dec < -6) dec = -6;
   if (dec > 9) dec = 9;

   _snprintf(dest, dest_size - 1, "%lG", round_to_nearest_double(Hz, -dec));

   return dest;
}

// *****************************************************************************
// Phase Noise specific calculations
// *****************************************************************************

void PN_calculate_rms_noise(
    const SINGLE *frequency,
    const DOUBLE *dBc_Hz,
    const S32    *valid,
    S32           n_points,
    S32           L_column,
    S32           U_column,
    DOUBLE        carrier_Hz,
    DOUBLE       &RMS_rads,
    DOUBLE       &CNR,
    DOUBLE       &resid_FM,
    DOUBLE       &jitter_s
)
{
   DOUBLE PM_sum = 0.0;
   DOUBLE FM_sum = 0.0;

   if (L_column < 0) L_column = 0;
   if (U_column >= n_points) U_column = n_points - 1;

   if (L_column == U_column)
   {
      if (valid[L_column])
      {
         PM_sum = pow(10.0, dBc_Hz[L_column] / 10.0);
         FM_sum = PM_sum * frequency[L_column] * frequency[L_column];
      }
   }
   else
   {
      for (S32 i = L_column; i < U_column; i++)
      {
         if (valid[i] && valid[i + 1])
         {
            DOUBLE V1, V2, V;
            DOUBLE F1 = frequency[i];
            DOUBLE F2 = frequency[i + 1];
            DOUBLE F = F1 + ((F2 - F1) / 2.0);

            // Interpolate midpoint in dB space, then convert to linear space and integrate
            V1 = dBc_Hz[i];
            V2 = dBc_Hz[i + 1];
            V = pow(10.0, (V1 + ((V2 - V1) / 2.0)) / 10.0);

            FM_sum += (V * (F * F) * (F2 - F1));
            PM_sum += (V * (F2 - F1));
         }
      }
   }

   CNR = (PM_sum == 0.0) ? 0.0 : (10.0 * log10(PM_sum));
   RMS_rads = sqrt(PM_sum * 2.0);
   resid_FM = sqrt(FM_sum * 2.0);
   
   if (carrier_Hz > 0.0)
      jitter_s = RMS_rads / (carrier_Hz * 3.14159265358979323846 * 2.0);
   else
      jitter_s = 0.0;
}

// *****************************************************************************
// Trace Smoothing
// *****************************************************************************

void PN_smooth_trace(
    const DOUBLE *VW,             // Array of linear noise values in watts (length: width)
    const S32    *VV,             // Array of validity flags (length: width)
    S32           width,          // Number of screen columns (array length)
    S32           smooth_samples, // Number of samples to smooth
    bool          alt_smoothing,  // Use alternative symmetric smoothing
    DOUBLE       *smoothed_dBc    // Output array (length: width)
)
{
    if (smooth_samples <= 0)
    {
        for (S32 col = 0; col < width; col++)
        {
            if (VV[col] && VW[col] > 0.0)
                smoothed_dBc[col] = 10.0 * log10(VW[col]);
            else
                smoothed_dBc[col] = 0.0;
        }
        return;
    }

    for (S32 col = 0; col < width; col++)
    {
        S32 min_required = 0;
        S32 missing      = 0;

        if (!alt_smoothing)
        {
            min_required = smooth_samples;
            missing      = 0;
        }
        else
        {
            min_required = 0;

            S32 start = col - smooth_samples;
            S32 end   = col + smooth_samples;

            S32 missing_from_left = 0;
            S32 missing_from_right = 0;
            S32 max_x = width - 1;

            if (end > max_x) end = max_x;
            if (start < 0)   { missing_from_left = -start; start = 0; }

            for (S32 i = start; i <= end; i++)
            {
                if (!VV[i])
                {
                    if (i < col) ++missing_from_left;
                    if (i > col) ++missing_from_right;
                }
            }

            missing = (std::max)(missing_from_left, missing_from_right);
        }

        S32 start = col - (smooth_samples - missing);
        S32 end   = col + (smooth_samples - missing);

        if (start < 0)       start = 0;
        if (end   > width - 1) end   = width - 1;

        DOUBLE acc = 0.0;
        S32    cnt = 0;

        for (S32 i = start; i <= end; i++)
        {
            if (VV[i])
            {
                acc += VW[i];
                cnt++;
            }
        }

        if (cnt > min_required && acc > 0.0)
        {
            acc /= cnt;
            acc = 10.0 * log10(acc);
            smoothed_dBc[col] = acc;
        }
        else if (VV[col] && VW[col] > 0.0)
        {
            smoothed_dBc[col] = 10.0 * log10(VW[col]);
        }
        else
        {
            smoothed_dBc[col] = 0.0;
        }
    }
}

// *****************************************************************************
// Spur Removal
// *****************************************************************************

void PN_clip_spurs(
    DOUBLE    *VD,       // Array of noise values in dB (length: width). Will be modified in place.
    const S32 *VV,       // Array of validity flags (length: width)
    S32        width,    // Number of screen columns (array length)
    S32        spur_dB   // Clipping threshold in dB
)
{
   S32 window = width / 25;

   for (S32 c=0; c < width; c++)
      {
      if (!VV[c])
         {
         continue;
         }

      DOUBLE left_min  = 100000.0;
      DOUBLE right_min = 100000.0;

      DOUBLE avg = 0.0;
      S32    cnt = 0;

      for (S32 j=1; j < window; j++)
         {
         S32 left  = c-j; if (left < 0)       left  = 0;
         S32 right = c+j; if (right >= width) right = width-1;

         if (VV[left]) 
            {
            avg += VD[left];
            ++cnt;

            if (VD[left] < left_min)
               {
               left_min = VD[left];
               }
            }
            
         if (VV[right])
            {
            avg += VD[right];
            ++cnt;

            if (VD[right] < right_min) 
               {
               right_min = VD[right];
               }
            }
         }                    

      if (((VD[c] - left_min)  >= spur_dB) && 
          ((VD[c] - right_min) >= spur_dB))
         {
         VD[c] = avg / cnt;
         }
      }
}
