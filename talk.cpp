#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "gpiblib.h"

void WINAPI GPIB_error(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl)
{
   printf("%s",msg);

   exit(1);
}

void shutdown(void)
{
   GPIB_disconnect();
}

void main(S32 argc, C8 **argv)
{
   setbuf(stdout,NULL);

   if (argc < 2)
      {
      printf("Usage: talk <address> [<command>] [<delay_msec>]\n\n");
      printf("Example: \n\n         talk 18 \"IP\" 100\n\n");
      printf("         Sends the string \"IP\" (Instrument Preset) to the instrument at address 18,\n");
      printf("         followed by a 100-millisecond delay before disconnecting\n\n");
      printf("Notes:\n\n         If <delay_msec> is omitted, a 500-millisecond delay will be used\n");
      exit(1);
      }

   atexit(shutdown);

   GPIB_connect(atoi(argv[1]),
                GPIB_error,
                0,
                3000);

   if (argc >= 3)
      {
      if (!GPIB_write(argv[2]))
         {
         printf("Write error!\n");
         exit(1);
         }

      S32 delay_ms = 500;

      if (argc >= 4)
         {
         sscanf(argv[3],"%d",&delay_ms);
         }

      Sleep(delay_ms);    // (500 ms needed for NI PCI-GPIB on HP 8753C
      }
}
