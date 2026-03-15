#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "typedefs.h"
#include "gpiblib.h"

const S32 MAX_FILE_BYTES = 1024*1024*4;

void WINAPI GPIB_error(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl)
{
   printf("Error: %s",msg);

   exit(1);
}

void shutdown(void)
{
   GPIB_disconnect();         
}

void main(S32 argc, C8 **argv)
{
   setbuf(stdout,NULL);
   
   if (argc != 3)
      {
      printf(
             "Usage: psaplot <address> <.GIF filename>\n\n"
             "Address is ignored if Ethernet connection is in use.  (Typically,\n"
             "CONNECT.INI's interface_settings line would be x.x.x.x:5025 in this case,\n"
             "with is_Prologix set to 0.  x.x.x.x is a numeric IP address on the LAN\n"
             "such as 192.168.1.239.)\n"
             "\n"
             "Example: psaplot 1 spectrum.gif\n"
             "         Retrieves a .GIF screenshot from the analyzer at address 1\n");
      exit(1);
      }

   atexit(shutdown);

   GPIB_connect(atoi(argv[1]),
                GPIB_error,
                0,
                50000);

   GPIB_auto_read_mode(FALSE);
   
   GPIB_set_EOS_mode(10);
   GPIB_set_serial_read_dropout(50000);

   //
   // Clear error status and show instrument ID
   //
                   
   GPIB_write("*CLS");
   GPIB_write("*IDN?");

   C8 ID[512] = { 0 };
   memcpy(ID, GPIB_read_ASC(512), sizeof(ID)-1);
   fprintf(stderr,"Connected to %s\n", ID);
   _strupr(ID);

   bool is_E4406 = (strstr(ID,"E4406") != NULL);
   bool is_RS    = (strstr(ID,"ROHDE") != NULL);

   if (is_E4406)
      {
      GPIB_write(":HCOPY:SDUMP:DATA? GIF");
      }

   if (is_RS)
      {
      GPIB_write("HCOP:DEST 'SYST:COMM:MMEM'");          // Works on FSP but generates corrupted .BMP file
      GPIB_write("HCOP:DEV:COL ON");
      GPIB_write("HCOP:DEV:LANG BMP");
      GPIB_write("HCOP:ITEM:ALL");
      GPIB_write("HCOP:MODE:SCR");
      GPIB_write("MMEM:NAME 'c:\\temp\\hcopy_dev.bmp'");
      GPIB_write("HCOP:IMM");
      GPIB_write("MMEM:DATA? 'c:\\temp\\hcopy_dev.bmp'");
      }
   else
      {
      GPIB_write(":MMEM:DEL \"C:SCREEN.GIF\"");          // Required for VT's NF analyzer (based on E4407A?), which won't overwrite an existing file
      GPIB_write(":MMEM:STOR:SCR \"C:SCREEN.GIF\"");     // Required for PSA support, tested OK on E4443A A.11.21 per N6OLD (1/26/20)
      GPIB_write(":MMEM:DATA? \"C:SCREEN.GIF\"");
      }

   S8 *preamble = (S8 *) GPIB_read_BIN(2);
   assert(preamble[0] == '#');
   S32 n_preamble_bytes = preamble[1] - '0';
   
   C8 *size_text = GPIB_read_ASC(n_preamble_bytes);
   S32 n_bytes = atoi(size_text);

   static S32 data[MAX_FILE_BYTES];

   memset(data, 
          0, 
          sizeof(data));
   
   if (n_bytes >= MAX_FILE_BYTES)
      {
      printf("File exceeds maximum size (%d bytes max, size = %d bytes)\n", n_bytes, MAX_FILE_BYTES);
      exit(1);	
      }

   printf("Reading %d bytes..", n_bytes);
   
   S32 offset = 0;
   S32 remaining = n_bytes;

   while (remaining > 0)
      {
      S32 cnt = min(remaining, 65536);

      printf(".");

      memcpy(&data[offset],
             GPIB_read_BIN(cnt,
                           TRUE,
                           FALSE), 
             cnt);

      offset += cnt;
      remaining -= cnt;
      }

   printf("\nWriting %s...\n", argv[2]);

   FILE *out = fopen(argv[2],"wb");
   
   if (out == NULL)
      {
      printf("Couldn't open %s for writing\n", argv[2]);
      exit(1);
      }

   fwrite(data, 1, n_bytes, out);

   fclose(out);

   if (!(is_E4406 || is_RS))
      {
      GPIB_write(":MMEM:DEL \"C:SCREEN.GIF\"");          // Needed to avoid error on E7402A (sjsmith, 1/15/21)
      }

   printf("Done, %d bytes written to %s\n", n_bytes, argv[2]);
}
