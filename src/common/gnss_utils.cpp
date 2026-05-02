#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "gnss_utils.h"
#include "timeutil.h"

DOUBLE ddmm2degrees(DOUBLE dm, C8 dir)
{
   DOUBLE degs = (DOUBLE) ((S32) dm / 100);
   DOUBLE mins = dm - (100.0 * degs);

   degs = degs + (mins / 60.0);

   if ((dir == 'S') || (dir == 'W') || (dir == 's') || (dir == 'w'))
      {
      degs *= -1.0;
      }

   return degs;
}

DOUBLE degrees2ddmm(DOUBLE degs, bool *neg)
{
   DOUBLE ddmm = degs;

   *neg = (degs <= 0.0);

   DOUBLE sgn = *neg ? -1.0 : 1.0;
   degs *= sgn;

   ddmm = floor(degs);
   DOUBLE frac = degs - ddmm;
   ddmm = (ddmm * 100.0) + (60.0 * frac);

   return ddmm;
}

bool parse_gpgga(const C8 *line, GpggaData &data)
{
   if (line == NULL) return false;
   
   // We expect the line to start with $GPGGA, (case-insensitive check handled by caller or here)
   if (_strnicmp(line, "$GPGGA,", 7) != 0) return false;

   int count = sscanf(line, "$GPGGA,%lf,%lf,%c,%lf,%c,%d,%d,%lf,%lf,%c,%lf,%c",
                      &data.UTC,
                      &data.latitude,
                      &data.lat_sign,
                      &data.longitude,
                      &data.long_sign,
                      &data.fix_quality,
                      &data.n_SV,
                      &data.HDOP,
                      &data.altitude,
                      &data.alt_units,
                      &data.geoid_sep,
                      &data.geoid_sep_units);

   return (count >= 6); // At least up to fix_quality
}

bool parse_gprmc(const C8 *line, GprmcData &data)
{
   if (line == NULL) return false;

   if (_strnicmp(line, "$GPRMC,", 7) != 0) return false;

   int count = sscanf(line, "$GPRMC,%lf,%c,%lf,%c,%lf,%c,%lf,%lf,%d,%lf,%c",
                      &data.UTC,
                      &data.status,
                      &data.latitude,
                      &data.lat_sign,
                      &data.longitude,
                      &data.long_sign,
                      &data.speed,
                      &data.track,
                      &data.date,
                      &data.mag_var,
                      &data.mag_var_dir);

   return (count >= 9); // At least up to date
}

bool verify_nmea_checksum(const C8 *line)
{
    if (line == NULL || line[0] != '$') return false;

    const C8 *chk_ptr = strrchr(line, '*');
    if (chk_ptr == NULL) return false;

    U32 chksum_received = 0;
    if (sscanf(chk_ptr + 1, "%2X", &chksum_received) != 1) return false;

    U32 chksum_calculated = 0;
    const C8 *ptr = line + 1; // Skip '$'

    while (ptr < chk_ptr)
    {
        chksum_calculated ^= (U8)(*ptr++);
    }

    return (chksum_calculated == chksum_received);
}

S64 nmea_to_us(S32 date, DOUBLE utc)
{
   // date is DDMMYY
   S32 day = date / 10000;
   S32 month = (date / 100) % 100;
   S32 year = date % 100;
   
   if (year < 70) year += 2000; else year += 1900;

   DOUBLE mjd = USTIMER::date_to_MJD(year, month, day);
   
   // utc is HHMMSS.sss
   S32 hms = (S32) utc;
   S32 hrs = hms / 10000;
   S32 mins = (hms / 100) % 100;
   S32 secs = hms % 100;
   DOUBLE frac = utc - floor(utc);
   
   DOUBLE seconds = (hrs * 3600.0) + (mins * 60.0) + secs + frac;
   mjd += seconds / 86400.0;
   
   return USTIMER::MJD_to_us(mjd);
}
