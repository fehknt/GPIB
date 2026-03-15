//
// Send string via COM port
//

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <float.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "typedefs.h"
#include "comport.cpp"

#define VERSION "1.01"

COMPORT serial;

void shutdown(void)
{
}

void main(int argc, char **argv)
{
   setbuf(stdout, NULL);

   if (argc < 4)
      { 
      fprintf(stderr,"Usage: txcom COM_port baud string [read]\n\n");
      fprintf(stderr,"Example: txcom com3 115200 \"mode cw\"\n");
      fprintf(stderr,"\n");
      fprintf(stderr,"If string contains '?', or if last argument is 'read'\n");
      fprintf(stderr,"the response will be read and displayed\n\n");
      exit(1);
      }

   atexit(shutdown);

   C8 *port = argv[1];

   //
   // Open port without DTR control to avoid resetting Arduinos, etc.
   //

   S32 bps = 115200;
   sscanf(argv[2],"%d",&bps);

   S32 rc = serial.connect(port, bps, FALSE);

   if (rc != 0)
      {
      fprintf(stderr,"Could not open %s\n", port);
      exit(1);
      }

   fprintf(stderr,"Transmitting '%s'\n", argv[3]);
   serial.printf("%s\r", argv[3]);

   //
   // If this was a query, get the response
   //

   if ((argc > 4) || (strstr(argv[3], "?") != NULL))
      printf("%s\n", serial.gets(-1, 1000, 250));
   else
      Sleep(100);    // CloseHandle() appears to corrupt outgoing data w/CP2102 if closed too soon after write?

   serial.disconnect();
}

