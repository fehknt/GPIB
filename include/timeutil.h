#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "typedefs.h"
// *****************************************************************************
//
// TIMEUTIL: Time/date-related utility classes
//
// john@miles.io 18-Mar-10
//
// *****************************************************************************

#pragma once
#include <time.h>

//
// Configuration
//

#ifndef TIMEUTIL_MAX_LOG_ENTRY_STRING
   #define TIMEUTIL_MAX_LOG_ENTRY_STRING 8192
#endif

//
// Globals
//

const S64    SECONDS_PER_MINUTE  =  60LL;
const S64    SECONDS_PER_HOUR    =  SECONDS_PER_MINUTE * 60LL;
const S64    SECONDS_PER_DAY     =  SECONDS_PER_HOUR   * 24LL;
const S64    SECONDS_PER_YEAR    = (SECONDS_PER_DAY    * 36524218967LL) / 100000000LL;

const S64 SYS_TIME_1970 = 11644473600LL; // seconds between Jan 1, 1601 and Jan 1, 1970, for NT/Unix time conversion
const S64 MJD_TIME_1970 = 40587LL;       // days between Nov 17, 1858 and Jan 1, 1970, for MJD/Unix time conversion
const S64 NTP_TIME_1970 = 2208988800LL;  // seconds between Jan 1, 1900 and Jan 1, 1970

#ifndef TIMEUTIL_THREAD_SAFE             // lock-protect us() and other calls unless explicitly requested not to
#define TIMEUTIL_THREAD_SAFE 1
#endif



class USTIMER
{
   BOOL   QPC_OK;
   DOUBLE q_multiplier;     
   S64    relative_time;   

   S64 q_first;    
   S32 t_first;
   S32 m_first;

   S64 q_last;    
   S32 t_last;
   S64 m_last;

   S64       last_result;
   DWORD_PTR thread_mask;
   
#if TIMEUTIL_THREAD_SAFE
   CRITICAL_SECTION time_lock;
#endif

   C8   logfile_name[MAX_PATH];
   BOOL log_previous_newline;
   C8   log_output_string[TIMEUTIL_MAX_LOG_ENTRY_STRING];

   S64 smallest_disagreement;
   S64 largest_disagreement;
   S32 num_disagreements;
   S64 largest_retrograde;
   S32 num_retrogrades;

   virtual void reset_timebase(void);

public:

   virtual void lock(void);

   virtual void unlock(void);

   USTIMER(BOOL force_single_core = FALSE);

   virtual ~USTIMER();

   //
   // High-resolution timer, similar to timeGetTime() but returns microsecond count
   // rather than milliseconds
   //
   // May be called safely from multiple threads
   //
   // Monotonic, but can potentially return zero delta times in successive calls
   // if the high-res timer needs to be corrected.  Clock rate can vary with 
   // power-management settings and PCI bus load (KB274323); the routine will do 
   // its best to compensate
   //
   // Timebase begins at zero at construction time, and (if QPF is supported with 
   // typical values of QPF=0x000000009000000) rolls over past 2^63 in approx. 290K years.
   // Consequently it's OK in most applications to use comparisons rather than 
   // subtraction when performing relative-time calculations
   //

  virtual S64 us(void);

   //
   // UTC-based file-modification time
   //
   // Returns # of 1-us intervals since 1-Jan-1601 UTC, or 
   // -1 on failure
   //

   static S64 file_time_us (C8 *filename, S64 *creation_time = NULL);

   //
   // UTC-based calendar time
   //
   // Returns # of 1-us intervals since 1-Jan-1601 UTC
   // 
   // This time is not guaranteed to increase monotonically, due to
   // DST or manual clock changes
   //

   static S64 system_time_us (void);

   //
   // Convert a file- or system-based time value in microseconds to
   // a locale-specific time string
   //

   static C8 *time_text (S64 time_us, 
                         C8 *text, 
                         S32 text_array_size);

   //
   // Convert a file- or system-based time value in microseconds to
   // a locale-specific date string
   //

   static C8 *date_text (S64  time_us, 
                         C8  *text, 
                         S32  text_array_size,
                         C8  *format = NULL);

   //                
   // Text date/timestamp
   //                

   static C8 *timestamp(C8 *text,
                        S32 text_array_size,
                        S64 at_time_us = 0);

   //
   // Duration string (Yy,Dd,Hh,Mm,Ss)
   //

   static C8 *duration_string(S64  us, 
                              C8  *text,
                              S32  text_array_size,
                              bool show_msec = FALSE,
                              bool show_mins = TRUE);

   //
   // Convert Julian date (JD) to ASCII timestamp
   //

   static C8 *JD_timestamp(C8    *text,
                           S32    text_array_size,
                           DOUBLE JD_time);

   //
   // Convert MJD to Windows file time (# of 1-us intervals since 1-Jan-1601 UTC)
   //

   static S64 MJD_to_us(DOUBLE MJD_time);

   //
   // Convert Windows file time to MJD
   //

   static DOUBLE us_to_MJD(S64 us);

   //
   // Get PC time (UTC) in fractional MJD format
   // >= 5 digits of precision are needed to represent whole-number seconds
   //
   // (Code from daytime.c by Tom Van Baak)
   //

   static DOUBLE current_MJD(SYSTEMTIME *st = NULL);

   //
   // Return whole-number Modified Julian Day (MJD) given
   // calendar year, month (1-12), and day (1-31).
   // - Valid for Gregorian dates from 17-Nov-1858.
   // - Adapted from sci.astro FAQ.
   //
   // (Code from daytime.c by Tom Van Baak)
   //

   static DOUBLE date_to_MJD(S32 year, S32 month, S32 day);

   //
   // Convert 64-bit fixed point NTP timestamp in network byte order (BE) 
   // to Windows file time
   //

   static S64 NTP_to_us(void *NTP_time);

   //
   // Set a new log filename
   //
   // By default, text passed to log_printf() will not be written
   // to a file
   //

   virtual void set_log_filename(C8 *filename);

   //
   // Write timestamped record to logfile, returning composited string that was
   // written 
   //
   // (If no logfile has been declared with set_log_filename(), the string will
   // still be returned)
   //
   // Warning: returned string is global to the TIMEUTIL instance
   //

   virtual C8 * __cdecl log_printf(C8 *fmt, ...);

   virtual C8 * __cdecl log_vprintf(C8 *fmt, va_list ap);
};

struct SCOPETIME
{
   C8  name[256];
   S64 start;
   USTIMER timer;

   SCOPETIME(C8 *_name=NULL);

   virtual ~SCOPETIME();
};

struct SHIFTTIME
{
   C8  name[256];
   S64 start;
   USTIMER timer;

   SHIFTTIME(C8 *_name=NULL);

   virtual ~SHIFTTIME();
};

