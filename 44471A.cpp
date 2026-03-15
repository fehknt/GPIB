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
      printf("Usage: atten_3488a <address> [<relay 0-9> <state 0|1>]\n");
      printf("\n");
      printf("- Assumes HP 44471A in slot 1\n");
      printf("- Run with address only for interactive control\n");
      exit(1);
      }

   atexit(shutdown);

   GPIB_connect(atoi(argv[1]),
                GPIB_error,
                0,
                3000,
                -1,
                0,
                4096,
                0);        // 0 required for Prologix support

   GPIB_write("CMON 1");

   if (argc >= 3)
      {
      S32 relay = atoi(argv[2]);
      S32 state = atoi(argv[3]);

      C8 o[32] = "OPEN 10x";  o[7] = relay + '0';
      C8 c[32] = "CLOSE 10x"; c[8] = relay + '0'; 

      if (state)
         GPIB_write(c);
      else
         GPIB_write(o);

      exit(0);
      }

   printf("Off/on:\n");    // TODO: ANSI colors
   printf("\n");
   printf("  0 / )\n");
   printf("  1 / !\n"); 
   printf("  2 / @\n"); 
   printf("  3 / #\n"); 
   printf("  4 / $\n"); 
   printf("  5 / %%\n"); 
   printf("  6 / ^\n"); 
   printf("  7 / &\n"); 
   printf("  8 / *\n"); 
   printf("  9 / (\n"); 
   printf("\n");
   printf("  Esc: Exit\n\n");

   for (;;)
      {
      C8 ch = _getch();

      if (ch == 27)
         {
         break;
         }

      S32 relay = 0;
      S32 state = 0;

      switch (ch)
         {
         case '0': relay = 0; state = 0; break;
         case ')': relay = 0; state = 1; break; 
         case '1': relay = 1; state = 0; break; 
         case '!': relay = 1; state = 1; break; 
         case '2': relay = 2; state = 0; break; 
         case '@': relay = 2; state = 1; break; 
         case '3': relay = 3; state = 0; break; 
         case '#': relay = 3; state = 1; break; 
         case '4': relay = 4; state = 0; break; 
         case '$': relay = 4; state = 1; break; 
         case '5': relay = 5; state = 0; break; 
         case '%': relay = 5; state = 1; break; 
         case '6': relay = 6; state = 0; break; 
         case '^': relay = 6; state = 1; break; 
         case '7': relay = 7; state = 0; break; 
         case '&': relay = 7; state = 1; break; 
         case '8': relay = 8; state = 0; break; 
         case '*': relay = 8; state = 1; break; 
         case '9': relay = 9; state = 0; break; 
         case '(': relay = 9; state = 1; break; 
         }

      C8 o[32] = "OPEN 10x";  o[7] = relay + '0';
      C8 c[32] = "CLOSE 10x"; c[8] = relay + '0'; 

      if (state)
         GPIB_write(c);
      else
         GPIB_write(o);
      }
      
}
