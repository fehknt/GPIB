//
// GPS: Fetch fixes from GPS or other GNSS receiver
//
// Author: John Miles, KE5FX (john@miles.io)
//

#include <stdio.h>
#include <conio.h>
#include "gpiblib.h"
#include "gpib_utils.h"
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "typedefs.h"
#include "gnss.h"

#define VERSION "1.02"

GNSS_STATE *state;

//*************************************************************
//
// Return numeric value of string, with optional
// location of first non-numeric character
//
/*************************************************************/



void shutdown(void)
{
   if (state != NULL)
      {
      GNSS_shutdown(state);
      state = NULL;
      }
}

void main(S32 argc, C8 **argv)
{
   setbuf(stdout,NULL);

   if (argc < 2)
      {
      printf("\nGPS version %s of %s by John Miles, KE5FX\n\nThis program acquires fixes from GPS or other GNSS receivers,\n"
             "writing them to stdout\n\n", VERSION, __DATE__);

      printf("Usage: gps [<commands>...]\n\n");
      printf("Examples:\n\n");
      printf("   gps -nmea:com2\n                              Connect to NMEA GPS receiver \n                              at a \"virtual\" (USB) serial port\n\n");
      printf("   gps -nmea:[com2:baud=4800 parity=n data=8 stop=1]\n\n                              Connect to NMEA GPS receiver \n                              at a physical COM2 port\n\n");
      printf("   gps -nmea:[ke5fx.dyndns.org:9999]\n\n                              Connect to NMEA GPS receiver \n                              at a specified TCP/IP address\n\n");
      printf("   gps -nmfile:[captured data.txt]\n\n                              Parse and display fix data from\n                              captured NMEA text file\n\n");
      printf("   gps -nmip:ke5fx.dyndns.org:9999 -nmout:[Text to send]\n\n                              Transmit ASCII string to connected NMEA source\n                              (works with -nmea or -nmip)\n\n");
      printf("   gps -nmea:com2 -gpgga:47.8,-122.1,100.0,12,34,56\n\n                              Transmit $GPGGA string with specified \n");
      printf("                              lat/long/alt/h/m/s, then exit\n");

      exit(1);
      }

   state = GNSS_startup();
   atexit(shutdown);

   //
   // Check for CLI options
   //

   C8 _lpCmdLine[MAX_PATH] = "";

   for (S32 i=1; i < argc; i++)
      {
      strcat(_lpCmdLine, argv[i]);
      strcat(_lpCmdLine, " ");
      }

   C8 lpCmdLine[MAX_PATH];
   C8 lpUCmdLine[MAX_PATH];

   memset(lpCmdLine, 0, sizeof(lpCmdLine));
   strcpy(lpCmdLine, _lpCmdLine);
   strcpy(lpUCmdLine, _lpCmdLine);
   _strupr(lpUCmdLine);

   C8 *option = NULL;
   C8 *end    = NULL;
   C8 *beg    = NULL;
   C8 *ptr    = NULL;

   while (1)
      {
      //
      // Remove leading and trailing spaces from command line
      //
      // Caution: Argument checks below must not accept strings from lpUCmdLine that would
      // also be caught by lpCmdLine, since text excised from the lpUCmdLine string
      // remains in lpCmdLine!
      //

      for (U32 i=0; i < strlen(lpCmdLine); i++)
         {
         if (!isspace(lpCmdLine[i]))
            {
            memmove(lpCmdLine, &lpCmdLine[i], strlen(&lpCmdLine[i])+1);
            break;
            }
         }

      C8 *p = &lpCmdLine[strlen(lpCmdLine)-1];

      while (p >= lpCmdLine)
         {
         if (!isspace(*p))
            {
            break;
            }

         *p-- = 0;
         }

      //
      // ...
      //

      //
      // Exit loop when no more CLI options are recognizable
      // (This should leave the GNSS-related options on the command line)
      //

      break;
      }

   //
   // Pass remaining command-line args to GNSS receiver access library
   //

   if (!GNSS_parse_command_line(state, lpCmdLine))
      {
      printf("%s\n",state->error_text);
      exit(1);
      }

   //
   // Connect to receiver 
   //

   if (!GNSS_connect(state))
      {
      printf("%s\n",state->error_text);
      exit(1);
      }

   //
   // Display location whenever a new fix comes in
   //

   printf("Press any key to stop...\n\n");

   for (;;)
      {
      if (_kbhit())
         {
         _getch();
         break;
         }

      Sleep(10);

      if (GNSS_update(state))      
         {
         printf("Fix %I64d: Lat %lf, long %lf, alt %lf\n", 
            state->fix_num,
            state->latitude,
            state->longitude,
            state->altitude_m);
         }
#if 0
      IFSHIFT
         {
         for (;;)
            {
            IFSHIFT
               {
               }
            else
               break;
            }

         printf("Sending\n");
         GNSS_send(state, "SYST:STAT?");
         }
#endif
      }
}
