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

#include "stdtpl.h"
#include "appfile.cpp"

KVSTORE<KVAL> CLI; 

ARG_STRING (CLI, addr,           "",         KVAL_REQUIRED);
ARG_S32    (CLI, EOS_char,       10,         0);
ARG_S32    (CLI, repeat,         0,          0);
ARG_S32    (CLI, interval_ms,    1000,       0);
ARG_S32    (CLI, timeout_ms,     5000,       0);
ARG_S32    (CLI, wait_ms,        100,        0);
ARG_STRING (CLI, logfile,        "",         0);
ARG_STRING (CLI, ID_cmd,         "*IDN?",    KVAL_BLANK_OK);
ARG_STRING (CLI, read_cmd,       "READ?",    KVAL_BLANK_OK);
ARG_STRING (CLI, connect_cmd,    "",         KVAL_BLANK_OK);
ARG_STRING (CLI, disconnect_cmd, "SYST:LOC", KVAL_BLANK_OK);
ARG_STRING (CLI, dtype,          "F",        0);
ARG_DOUBLE (CLI, offset,         0.0,        0);
ARG_DOUBLE (CLI, scale,          1.0,        0);

struct CONN : public IPCONN 
{
   virtual void message_sink(IPMSGLVL level,   
                             C8      *text)
      {
      fprintf(stderr,"%s\n",text);
      }
}
IPC;

void kill_trailing_whitespace(C8 *dest)
{
   S32 l = strlen(dest);

   while (--l >= 0)
      {
      if (!isspace(((U8 *) dest)[l]))
         {
         dest[l+1] = 0;
         break;
         }
      }
}

void shutdown(void)
{
   if (IPC.status() && ARG_disconnect_cmd[0])
      {
      IPC.send_printf("%s\r\n", ARG_disconnect_cmd);
      }

   _fcloseall();
}

void main(S32 argc, C8 **argv)
{
   C8 *usage = "Usage: readinst /addr:<IP_address:[port]> [optional arguments]\n\nArguments:\n"
               "             /addr:<IP_address[:port]>     Example: /addr:192.168.1.53:5025\n"
               "         /EOS_char:<num>                   Default: $\n"
               "           /repeat:<num>                   Default: $ (0=until stopped)\n"
               "           /ID_cmd:<cmd>                   Default: \"$\"\n"
               "         /read_cmd:<cmd>                   Default: \"$\"\n"
               "      /connect_cmd:<cmd>                   Default: \"$\"\n"
               "   /disconnect_cmd:<cmd>                   Default: \"$\"\n"
               "      /interval_ms:<num>                   Default: $\n"
               "       /timeout_ms:<num>                   Default: $\n"
               "          /wait_ms:<num>                   Default: $\n"
               "          /logfile:<filename>              Default: \"$\"\n"
               "            /dtype:<F|A>                   Default: $ (F=floats, A=plaintext)\n"
               "           /offset:<val>                   Default: $.0\n"
               "            /scale:<val>                   Default: $.0\n";

   if (!CLI.from_args(argc, argv, usage))
      {
      exit(1);
      }

   S64 timeout_us          = ARG_timeout_ms  * 1000LL;
   S64 reading_interval_us = ARG_interval_ms * 1000LL;
   
   FILE *outfile = NULL;

   if (ARG_logfile[0])
      {
      outfile = fopen(ARG_logfile,"wt");
      setbuf(outfile, NULL);
      }

   atexit(shutdown);

   IPC.connect(ARG_addr, 5025);

   Sleep(ARG_wait_ms);

   if (ARG_connect_cmd[0])
      {
      IPC.send_printf("%s\r\n", ARG_connect_cmd);
      }

   Sleep(ARG_wait_ms);

   const C8 *query = "READ?";

   if (ARG_ID_cmd[0])
      IPC.send_printf("%s\r\n", ARG_ID_cmd);
   else
      IPC.send_printf("%s\r\n", query);

   Sleep(ARG_wait_ms);

   C8 line[512];
   S32 len = 0;

   S32 lines_received = 0;

   USTIMER timer;

   S64 start_time      = timer.us();
   S64 last_line_time  = start_time;
   S64 next_reading_us = start_time + reading_interval_us;

   bool done = FALSE;

   while (!done)
      {
      S64 cur_us = timer.us();

      if ((_kbhit()) && (_getch() == 27))
         {
         break;
         }

      if ((timeout_us > 0) && ((cur_us - last_line_time) >= timeout_us))
         {
         break;
         }

      if (!IPC.status())
         {
         exit(1);
         }

      S32 avail = IPC.receive_poll();

      if (avail)
         {
         for (S32 i=0; i < avail; i++)
            {
            if ((_kbhit()) && (_getch() == 27))
               {
               done = 1;
               break;
               }

            C8 ch = 0;
            assert(IPC.read_block(&ch, 1));

            line[len++] = ch;

            if ((ch == ARG_EOS_char) || (len == (sizeof(line)-1)))   
               {
               line[len] = 0;
               len = 0;

               if (((ARG_dtype[0] == 'F') || (ARG_dtype[0] == 'f'))
                    &&
                   (ARG_ID_cmd[0] == 0))
                  {
                  DOUBLE val = DBL_MAX;
                  sscanf(line,"%lf", &val); 

                  if (val == DBL_MAX)
                     strcpy(line,"(Invalid reading)\n");
                  else
                     sprintf(line, "%+.16E", (val * ARG_scale) + ARG_offset);
                  }
               else
                  {
                  kill_trailing_whitespace(line);

                  if (ARG_ID_cmd[0])
                     {
                     printf("%s response: ",ARG_ID_cmd);
                     }
                  }

               printf("%s\n", line);

               if ((outfile != NULL) && !ARG_ID_cmd[0])
                  {
                  fprintf(outfile,"%s\n", line);
                  }

               ARG_ID_cmd[0] = 0;

               last_line_time = timer.us();

               if ((++lines_received >= ARG_repeat) && (ARG_repeat != 0))
                  {
                  done = 1;
                  break;
                  }

               if ((timeout_us > 0) && ((timer.us() - last_line_time) >= timeout_us))
                  {
                  done = 1;
                  break;
                  }
               }
            }
         }

      if (done)
         {
         break;
         }

      if (cur_us < next_reading_us)                // Read at EXACTLY the specified interval as determined by PC clock, 
         {                                         // regardless of measurement or communication overhead
         Sleep(10);
         continue;
         }

      next_reading_us += reading_interval_us;      

      IPC.send_printf("%s\r\n", query);            // Also note that read query is sent even if we never got an answer
      Sleep(ARG_wait_ms);                          // to the last one
      }

   if (lines_received > 0)
      {
      DOUBLE Hz = ((DOUBLE) lines_received * 1E6) / ((DOUBLE) (timer.us() - start_time));
      fprintf(stderr,"\nDone, %d line(s) received\nAverage rate = %.3lf/sec\n",lines_received, Hz);
      }
}
