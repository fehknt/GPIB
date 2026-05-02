#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#include "typedefs.h"
#include "recorder.h"

RECORDER R;
 
void main(int argc, char **argv)
{
   if (argc < 6) 
      {
      printf("Usage: ssmchan filename.ssm center_Hz span_Hz RBW_Hz offset_dB\n");
      exit(1);
      }

   DOUBLE center_Hz = 1E9;
   DOUBLE span_Hz   = 1E6;
   DOUBLE RBW_Hz    = 1000.0;
   DOUBLE offset_dB = 0.0;

   sscanf(argv[2], "%lf", &center_Hz);
   sscanf(argv[3], "%lf", &span_Hz);
   sscanf(argv[4], "%lf", &RBW_Hz);
   sscanf(argv[5], "%lf", &offset_dB);

   DOUBLE min_bin_Hz = center_Hz - (span_Hz/2.0);
   DOUBLE max_bin_Hz = center_Hz + (span_Hz/2.0);

   S32 file_version = 0;
   DOUBLE freq_start = 0.0;
   DOUBLE freq_end = 0.0;
   SINGLE min_dBm = 0.0F;
   SINGLE max_dBm = 0.0F;
   S32    n_amplitude_levels = 0;
   S32    dB_per_division = 0;
   SINGLE top_cursor_dBm = 0.0F;
   SINGLE bottom_cursor_dBm = 0.0F;
   VFX_RGB palette[256] = { 0 };

   S32 width = R.open_readable_file(argv[1],
                                  &file_version,
                                  &freq_start,
                                  &freq_end,              
                                  &min_dBm,               
                                  &max_dBm,               
                                  &n_amplitude_levels,    
                                  &dB_per_division,       
                                  &top_cursor_dBm,        
                                  &bottom_cursor_dBm,     
                                  palette,               
                                  256);

   DOUBLE Hz_per_bin = (freq_end - freq_start) / width; 

   S32 n_records = R.n_readable_records();

   SINGLE dBm_bins[65536] = { 0 };
   C8     caption[128] = { 0 };
   DOUBLE latitude      = 0.0;
   DOUBLE longitude     = 0.0; 
   DOUBLE altitude_m    = 0.0; 
   DOUBLE start_Hz      = 0.0;
   DOUBLE stop_Hz       = 0.0;

   time_t start_time = (time_t) 0;

   printf("X,Y\n");

   for (S32 i=0; i < n_records; i++)
      {
      C8 caption[128] = { 0 };

      time_t acquisition_time = R.read_record(dBm_bins,
                                              i,
                                             &latitude,
                                             &longitude,
                                             &altitude_m,
                                             &start_Hz,
                                             &stop_Hz,
                                              caption,
                                              sizeof(caption));

      if (i == 0)
         {
         start_time = acquisition_time;
         }

      DOUBLE Hz = freq_start;
      DOUBLE norm_BW = 10.0 * log10(RBW_Hz);

      DOUBLE avg = 0.0;
      DOUBLE pk  = 0.0;

      for (S32 i=0; i < width; i++, Hz += Hz_per_bin)
         {
         if ((Hz < min_bin_Hz) || (Hz > max_bin_Hz)) 
            {
            continue;
            }

         DOUBLE w = pow(10.0, (dBm_bins[i] - norm_BW) / 10.0);    // bin power in watts normalized to 1 Hz BW

         if (w > pk) pk = w;
         avg += w;
         }

      DOUBLE avg_pwr = 10.0 * log10(avg);
      DOUBLE peak_pwr = 10.0 * log10(pk);

      printf("%lf,%lf\n", difftime(acquisition_time, start_time)/60.0, avg_pwr-offset_dB);
      }
}
