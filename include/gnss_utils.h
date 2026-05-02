#ifndef GNSS_UTILS_H
#define GNSS_UTILS_H

#include "typedefs.h"

// Coordinate conversion
DOUBLE ddmm2degrees(DOUBLE dm, C8 dir);
DOUBLE degrees2ddmm(DOUBLE degs, bool *neg);

// NMEA sentence parsing
struct GpggaData {
    DOUBLE UTC;
    DOUBLE latitude;
    C8 lat_sign;
    DOUBLE longitude;
    C8 long_sign;
    S32 fix_quality;
    S32 n_SV;
    DOUBLE HDOP;
    DOUBLE altitude;
    C8 alt_units;
    DOUBLE geoid_sep;
    C8 geoid_sep_units;
};

struct GprmcData {
    DOUBLE UTC;
    C8 status;
    DOUBLE latitude;
    C8 lat_sign;
    DOUBLE longitude;
    C8 long_sign;
    DOUBLE speed;
    DOUBLE track;
    S32 date;
    DOUBLE mag_var;
    C8 mag_var_dir;
};

bool parse_gpgga(const C8 *line, GpggaData &data);
bool parse_gprmc(const C8 *line, GprmcData &data);
bool verify_nmea_checksum(const C8 *line);

// Time synchronization
S64 nmea_to_us(S32 date, DOUBLE utc);

#endif
