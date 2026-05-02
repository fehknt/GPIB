//
// SATRACE: Fetch spectrum analyzer trace(s) 
//
// Author: John Miles, KE5FX (john@miles.io)
//

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "typedefs.h"
#include "gpiblib.h"
#include "specan.h"

#include "satrace_utils.h"

SA_STATE *state;
S32 addr_specan = -1;

void shutdown(void);

void WINAPI GPIB_error(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl)
 {
  printf("%s", msg);
  shutdown();
  exit(1);
}

void shutdown(void) {
  // Reconnect to analyzer before calling SA_shutdown
  // so that it can send the continuous-sweep command
  // without triggering a viWrite Error on a disconnected instrument
  GPIB_connect(addr_specan, NULL, 0, 10000);

  SA_shutdown();
  GPIB_disconnect();
}

int main(S32 argc, C8 **argv) {
   setbuf(stdout,NULL);

   if (argc < 2)
      {
      printf("\nSATRACE version %s of %s by John Miles, KE5FX\n\nThis program acquires one or more spectrum analyzer traces, writing\n"
             "frequency/amplitude pairs to stdout with optional resampling\n\n", VERSION, __DATE__);

      printf("Usage: satrace <address> [<commands>...]\n\n");
      // ... [rest of help output truncated for brevity in replacement, but I will keep it in actual edit]
      // Wait, I should keep the help output.

      printf("Examples using GPIB address 18:\n\n");
      printf("satrace 18               Auto-identify analyzer at GPIB address 18 and acquire\n"
             "                         a single trace from it\n\n");
      printf("satrace -sa44            Special option required for use with USB-SA44/SA124 \n"
             "                         Signal Hound  (No GPIB address is needed)\n\n");
      printf("satrace 18 -856xa        Special option required for use with HP8566A-HP8568A\n\n");
      printf("satrace 18 -8569b        Special option required for use with HP8569B/8570A\n\n"); 
      printf("satrace 18 -358xa        Special option required for use with HP3588A/3589A\n\n");           
      printf("satrace 18 -3585         Special option required for use with HP3585A/B\n\n"); 
      printf("satrace 18 -advantest    Special option required for use with supported \n"
             "                         Advantest R3200/R3400-series analyzers\n\n");
      printf("satrace 18 -r3261        Special option required for use with R3261/R3361\n\n");
      printf("satrace 18 -scpi         Special option required for use with Agilent E4400-\n"
             "                         series and other SCPI-compatible analyzers\n\n");
      printf("satrace 18 -f            Favor speed over resolution, CRT updates, or other\n"
             "                         factors (if supported)\n\n"); 
      printf("satrace 18 -t            Disable GPIB timeout checking during long sweeps\n\n"); 
      printf("satrace 18 -ao:-7        Add -7 dBm to all reported amplitude values\n\n");
      printf("satrace 18 -fo:150000000 Add 150 MHz to all reported frequency values\n\n");
      printf("satrace 18 -reps:15      Acquire 15 successive traces (0 = run until keypress)\n\n");
      printf("satrace 18 -trace:\"TA\"   Acquire from specified trace (HP 8568B/8566B only,\n"
             "                         default=\"TA\")\n\n");
      printf("satrace 18 -header       Display time/datestamp and available analyzer control\n"
             "                         settings\n\n");
      printf("satrace 18 -lf           Separate frequency/amplitude pairs with linefeeds\n"
             "                         rather than commas\n\n"); 
      printf("satrace 18 -spline:800   Resample trace using cubic spline reconstruction to\n"
             "                         generate (e.g.) 800 points, regardless of the\n"
             "                         analyzer's trace array width\n\n");
      printf("satrace 18 -point:128    Resample trace using point-sampled values\n\n");
      printf("satrace 18 -min:128      Resample trace using minimum bucket values when more\n"
             "                         source points than requested are available\n"
             "                         (otherwise use spline)\n\n");
      printf("satrace 18 -max:128      Resample trace using maximum bucket values when more\n"
             "                         source points than requested are available\n"
             "                         (otherwise use spline)\n\n");
      printf("satrace 18 -avg:128      Resample trace using averaged bucket values when more\n"
             "                         source points than requested are available\n"
             "                         (otherwise use spline)\n\n");
      printf("satrace 18 -connect:\"xxx\" \n"
             "                         Specify GPIB command string to be issued when \n"
             "                         initially connecting to instrument\n\n");
      printf("satrace 18 -disconnect:\"xxx\" \n"
             "                         Specify GPIB command string to be issued when\n"
             "                         disconnecting prior to program termination\n\n");
      printf("satrace 18 -856xa -f     Example using multiple options\n\n"); 

      printf("Additional control options supported by Signal Hound only:\n\n");

      printf("     -RL:-40             Specify reference level in dBm (default = -30)\n");
      printf("     -CF:90.3E6          Specify center frequency in Hz (default = 900 MHz)\n"); 
      printf("     -span:10E6          Specify span in Hz (default = 1 MHz)\n"); 
      printf("     -start:88E6         Specify sweep start frequency (default = 899 MHz)\n"); 
      printf("     -stop:108E6         Specify sweep stop frequency (default = 901 MHz)\n"); 
      printf("     -bins:64            Specify FFT kernel size (power of 2 from 16 to 256,\n                         default = 128)\n"); 
      printf("     -sens:2             Specify sensitivity factor (default = 2, range 0-2)\n"); 
      printf("     -RFATT:10           Specify RF attenuation in dB (0-15 dB in 5-dB steps,\n                         default = 0)\n"); 

      exit(1);
      }

   //
   // Check for CLI options
   //

   SatraceOptions opt;
   parse_satrace_options(opt, GetCommandLineA());

   S32 reps              = opt.reps;
   S32 n_dest_pts        = opt.n_dest_pts;
   RESAMPLE_OP resamp_op = opt.resamp_op;
   S32 LF_separator      = opt.LF_separator;
   S32 show_header       = opt.show_header;
   C8 *lpCmdLine         = opt.lpCmdLine;

   //
   // Pass remaining command-line args to spectrum-analyzer access library
   // Whatever is left will be treated as the GPIB address
   //
   // Note that specan.dll doesn't modify the command line passed to it, so 
   // the GPIB address must still appear first on the line
   //

   state = SA_startup();
   atexit(shutdown);

   SA_parse_command_line(lpCmdLine);

    addr_specan = (S32)ascnum(lpCmdLine, 10);

    if (((!lpCmdLine[0]) || (!addr_specan)) && (!state->SA44_mode)) {
      printf("Error -- must specify GPIB address\n");
      exit(1);
    }

    //
    // Connect to analyzer and show header if requested
    //

    if (!SA_connect(addr_specan, (C8 *)lpCmdLine, GPIB_error)) {
      printf("Error identifying analyzer at address %s\n", argv[1]);
      exit(1);
    }

   static StdoutOutput stdout_out;
   if (show_header)
      {
      TraceProcessor::PrintHeader(stdout_out, state);
      }

   //
   // Fetch and display requested trace(s) 
   //

   for (S32 n=0; (reps == 0) ? TRUE : (n < reps); n++)
      {
      if (_kbhit())
         {
         _getch();
         break;
         }

      if (show_header)
         {
         time_t acq_time = 0;
         time(&acq_time);
                                  
         printf("\ntrace_num %d\n", n);
         printf("acquisition_time %s",ctime(&acq_time));
         }

      SA_fetch_trace();

      printf("\n");

      DOUBLE *src       = state->dBm_values;
      S32     n_src_pts = state->n_trace_points;  

      if (n_dest_pts != -1)      // was resampling requested?
         {
         static DOUBLE dest[65536];

         if ((n_dest_pts < 1) || (n_dest_pts > 65536))
            {
            printf("Error -- point count must be from 1 to 65536\n");
            exit(1);
            }

         SA_resample_data(src,  n_src_pts,
                          dest, n_dest_pts,
                          resamp_op);

         src       = dest;       // use resampled trace data
         n_src_pts = n_dest_pts;
         }

      if (show_header) printf(LF_separator ? "trace_data\n":"trace_data ");

      for (S32 i=0; i < n_src_pts; i++)
         {
         DOUBLE f = TraceProcessor::CalculateBinFrequency(state->min_Hz, state->max_Hz, n_src_pts, i);
         printf("%lf,%lf",f, src[i]);            // freq in Hz, amplitude in dBm

         if (i != n_src_pts-1)
            {
            printf(LF_separator ? "\n" : ", ");
            }

         }

      if (n != reps-1)
         {
         printf(LF_separator ? "\n" : "\n");
         }
      }

   SA_disconnect(TRUE);
   return 0;
}
