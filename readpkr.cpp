#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "typedefs.h"
#include "timeutil.cpp"

#define IPCONN_DEFAULT_RECV_BUF_BYTES 32768
#define IPCONN_SO_RECVBUF_BYTES       32768

#include "ipconn.cpp"

struct CONN : public IPCONN 
{
   virtual void message_sink(IPMSGLVL level,   
                             C8      *text)
      {
      fprintf(stderr,"%s\n",text);
      }
}
IPC;

void shutdown(void)
{
   _fcloseall();
}

void main(S32 argc, C8 **argv)
{
   if (argc < 2)
      {
      printf("Usage: readpkr <IP_address[:port]> [output_to_file.txt]\n\n");
      printf("Example: readpkr 192.168.1.221:5025 pressure.txt\n\n");
      exit(1);
      }

   S32 EOS = 10;
   S32 repeat_cnt = 0;
   S64 timeout_usec = 200000;
   S32 wait_msec = 0;
   S32 xmit_wait_ms = 0;
   DOUBLE offset_value = 0.0;
   
   FILE *outfile = NULL;

   if (argc > 2)
      {
      outfile = fopen(argv[2],"wt");
      setbuf(outfile, NULL);
      }

   atexit(shutdown);
   Sleep(wait_msec);

   IPC.connect(argv[1], 1298);

   const C8 *query = "READ?";

   fprintf(stderr,"Transmitting \"%s\" ...\n", query);

   IPC.send_printf("%s\r\n", query);
   Sleep(xmit_wait_ms);

   C8 incoming_line[512];
   S32 len = 0;

   S32 lines_received = 0;

   USTIMER timer;

   S64 start_time = timer.us();
   S64 last_line_time = start_time;

   BOOL done = FALSE;

   printf("Wait\n");
   while (!done)
      {
      if ((_kbhit()) && (_getch() == 27))
         {
         break;
         }

 //     if ((timeout_usec > 0) && ((timer.us() - last_line_time) >= timeout_usec))
 //        {
 //        break;
 //        }

      if (!IPC.status())
         {
         printf("Bail 1\n");
         exit(1);
         }

      S32 avail = IPC.receive_poll();

      if (avail == 0)
         {
         Sleep(10);
         continue;
         }
      
      for (S32 i=0; i < avail; i++)
         {
         if ((_kbhit()) && (_getch() == 27))
            {
            done = 1;
            break;
            }

         C8 ch = 0;
         assert(IPC.read_block(&ch, 1));

         incoming_line[len++] = ch;

         if ((ch == EOS) || (len == (sizeof(incoming_line)-1)))
            {
            incoming_line[len] = 0;
            len = 0;

            DOUBLE val = DBL_MAX;
            sscanf(incoming_line,"%lf", &val); 

            if (val != DBL_MAX)
               {
               DOUBLE torr = pow(10.0, (1.667 * val) - 11.46);
               DOUBLE mbar = pow(10.0, (1.667 * val) - 11.33);

               sprintf(incoming_line, "%.2lE torr, %.2lE mbar\n", torr, mbar);

               printf("%s", incoming_line);

               if (outfile != NULL)
                  {
                  fprintf(outfile,"%s", incoming_line);
                  }
               }

            last_line_time = timer.us();

            if ((++lines_received >= repeat_cnt) && (repeat_cnt != 0))
               {
               done = 1;
               break;
               }

            if ((timeout_usec > 0) && ((timer.us() - last_line_time) >= timeout_usec))
               {
               done = 1;
               break;
               }

            IPC.send_printf("%s\r\n", query);
            Sleep(xmit_wait_ms);
            }
         }
      }

   if (lines_received > 0)
      {
      DOUBLE Hz = ((DOUBLE) lines_received * 1E6) / ((DOUBLE) (timer.us() - start_time));
      fprintf(stderr,"\nDone, %d line(s) received\nAverage rate = %.3lf/sec\n",lines_received, Hz);
      }
}
