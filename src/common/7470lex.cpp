#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include "typedefs.h"
#include "w32sal.h"
#include "7470lex.h"

#include "gpib_utils.h"

// Global variables
C8 *plot_data = NULL;
C8 *plot_filename = NULL;
C8 *plot_end = NULL;
C8 *ptr = NULL;
S32 MaxX = 0, MinX = 0, MaxY = 0, MinY = 0;

C8 *COMMAND_names[CMD_END_OF_DATA] = 
{ 
  "CA",
  "CP",
  "CS",
  "DC",
  "DF",
  "DI",
  "DP",
  "DR",
  "DT",
  "IM",
  "IN",
  "IP",
  "IW",
  "LB",
  "LT",
  "OA",
  "OC",
  "OD",
  "OE",
  "OF",
  "OI",
  "OO",
  "OP",
  "OS",
  "OW",
  "PA",
  "PD",
  "PR",
  "PU",
  "SA",
  "SC",
  "SI",
  "SL",
  "SM",
  "SP",
  "SR",
  "SS",
  "TL",
  "UC",
  "VS",
  "XT",
  "YT",
  "CI",
  "AA",
  "AR",
  "(Unknown)",
};

//****************************************************************************
//
// Return TRUE if integer or float value follows
//
//****************************************************************************

S32 next_number(void)
{
   if ((isdigit((U8) *ptr)) || (*ptr == ' '))
      {
      return TRUE;
      }

   if ((*ptr == '-') || (*ptr == '+') || (*ptr == '.'))
      {
      return TRUE;
      }

   return FALSE;
}

//****************************************************************************
// 
// Return TRUE if comma follows
//
//****************************************************************************

S32 next_comma(void)
{
   for (;;)
      {
      C8 token = *ptr;

      if (token == ',') 
         {
         return TRUE;
         }

      if (token == ' ')
         {
         ptr++;
         continue;
         }

      return FALSE;
      }
}

//****************************************************************************
//
// Return valid command, or error code if unrecognized
//
//****************************************************************************

COMMAND next_command(void)
{
   //
   // Look for supported command
   //

   if ((plot_end - ptr) < 2)
      {
      return CMD_END_OF_DATA;
      }

   for (S32 i=0; i < CMD_UNKNOWN; i++)
      {
      if ((COMMAND_names[i][0] == toupper(ptr[0])) &&
          (COMMAND_names[i][1] == toupper(ptr[1])))
         {
         return (COMMAND) i;
         }
      }

   //
   // Supported command not found
   //

   return CMD_UNKNOWN;
}

//****************************************************************************
//
// Return next command, swallowing remainder of existing command if any
//
//****************************************************************************

COMMAND fetch_command(void)
{
   while (ptr < plot_end)
      {
      COMMAND result = next_command();

      if (result == CMD_END_OF_DATA)
         {
         return result;
         }

      if (result != CMD_UNKNOWN)
         {
         ptr += 2;
         return result;
         }

      ++ptr;
      }

   return CMD_END_OF_DATA;
}

//****************************************************************************
//
// Fetch comma
//
//****************************************************************************

void fetch_comma(void)
{
   //
   // Problem cases:
   //
   //    Tek 2710 requires us to accept spaces as delimiters (e.g., "PA 50 50;")
   //    HP 8510B requirs us to accept one or more in a row (e.g., "SC-396 ,3971 ,-50 ,404 ;")
   //

   S32 n = 0;
   
   while (next_comma())
      {
      ++ptr;
      ++n;
      }

   if (n == 0)
      {
      SAL_alert_box(plot_filename, "Missing comma -- malformed data?");
      exit(1);
      }
}


//****************************************************************************
//
// Swallow character, if present -- otherwise, return without doing anything
// Needed to work around HP 5372A firmware bug ("PAxxx,;yyyPD")
//
//****************************************************************************

S32 swallow_character(C8 chr)
{
   if (*ptr == chr)
      {
      ++ptr;
      return TRUE;
      }

   return FALSE;
}

//****************************************************************************
//
// Fetch integer value
//
//****************************************************************************

S32 fetch_integer(void)
{
   C8 *end;

   S32 val = ascnum(ptr, 10, &end);

   if (end == ptr)
      {
      SAL_alert_box(plot_filename, "Missing integer value -- malformed data?");
      exit(1);
      }

   ptr = end;

   return val;
}

//****************************************************************************
//
// Fetch float value
//
//****************************************************************************

SINGLE fetch_float(void)
{
   C8 *start = ptr;

   C8 text[256];
   S32 i = 0;

   while (1)
      {
      if (isdigit((U8) *ptr))
         {
         text[i++] = *ptr++;
         continue;
         }

      if ((*ptr == ' ') && (i == 0))
         {
         //
         // Leading spaces need to be skipped; spaces anywhere else
         // signal the end of the number
         //

         *ptr++;
         continue;
         }

      if (*ptr == '-')
         {
         text[i++] = *ptr++;
         continue;
         }

      if (*ptr == '+')
         {
         text[i++] = *ptr++;
         continue;
         }

      if (*ptr == 'E')
         {
         text[i++] = *ptr++;
         continue;
         }

      if (*ptr == '.')
         {
         text[i++] = *ptr++;
         continue;
         }

      break;
      }

   if (start == ptr)
      {
      SAL_alert_box("Error", "Missing float value -- malformed data?");
      exit(1);
      }

   SINGLE val;

   text[i] = 0;

   sscanf(text, "%f", &val);

   return val;
}

//****************************************************************************
//
// Fetch label text as zero-terminated ASCII string
// (Warning: modifies input data!)
//
//****************************************************************************

C8 *fetch_text(S32 terminator)
{
   //
   // Look for 0x03 or 0x00 terminator, or end of data
   //

   C8 *start = ptr;

   while (1)
      {
      if ((*ptr == terminator) || (*ptr == 0x00))
         {
         *ptr++ = 0;
         return start;
         }

      if (ptr >= plot_end)
         {
         SAL_alert_box("Error", "End of source data within label text -- malformed data?");
         exit(1);
         }

      ++ptr;
      }

   return start;
}

//****************************************************************************
//*                                                                          *
//*  Read all or part of a file into memory, returning memory location       *
//*  or NULL on error                                                        *
//*                                                                          *
//*  Memory will be allocated if dest==NULL                                  *
//*                                                                          *
//****************************************************************************

void * FILE_read (C8     *filename,     
                  S32    *len_dest,
                  void   *dest,
                  S32     len,
                  S32     start_offset)

{
   HANDLE handle;
   U32    n_bytes;
   U32    nbytes_read;
   S32    result;
   void  *buf;

   //
   // Open file
   //

   handle = CreateFile(filename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);

   if (handle == INVALID_HANDLE_VALUE)
      {
      return NULL;
      }

   //
   // Get its size
   //

   S32 actual_len = GetFileSize(handle, NULL);

   if (len_dest != NULL)
      {
      *len_dest = actual_len;
      }

   //
   // Set pointer to beginning of range
   //

   if (SetFilePointer(handle,
                      start_offset,
                      NULL,
                      FILE_BEGIN) == 0xffffffff)
      {
      CloseHandle(handle);
      return NULL;
      }

   //
   // Allocate memory for file range
   //

   n_bytes = len;

   if (n_bytes == 0xffffffff)
      {
      n_bytes = actual_len - start_offset;
      }

   buf = (dest == NULL) ? malloc(n_bytes) : dest;

   if (buf == NULL)
      {
      CloseHandle(handle);
      return NULL;
      }

   //
   // Read range
   //

   result = ReadFile(handle,
                     buf,
                     n_bytes,
          (LPDWORD) &nbytes_read,
                     NULL);

   CloseHandle(handle);

   if ((!result) || (nbytes_read != n_bytes))
      {
      if (dest != buf)
         {
         free(buf);
         }

      return NULL;
      }   

   return buf;
}
