#include "timeutil.h"

// *****************************************************************************
// USTIMER implementation
// *****************************************************************************

void USTIMER::reset_timebase(void)
{
   QPC_OK       = FALSE;
   q_multiplier = 0.0;
   q_first      = 0;
   m_first      = timeGetTime();

   S64 temp = 0;

   if (QueryPerformanceFrequency((LARGE_INTEGER *) &temp))
      {
      QPC_OK       = TRUE;
      q_multiplier = 1E6 / ((DOUBLE) temp);

      QueryPerformanceCounter((LARGE_INTEGER *) &q_first);
      }

   q_last = q_first;
   m_last = m_first;
}

void USTIMER::lock(void)
{
#if TIMEUTIL_THREAD_SAFE
   EnterCriticalSection(&time_lock);
#endif
}

void USTIMER::unlock(void)
{
#if TIMEUTIL_THREAD_SAFE
   LeaveCriticalSection(&time_lock);
#endif
}

USTIMER::USTIMER(BOOL force_single_core)
{
#if TIMEUTIL_THREAD_SAFE
   InitializeCriticalSection(&time_lock);
#endif

   thread_mask = 0;

   if (force_single_core)
      {
      thread_mask = SetThreadAffinityMask(GetCurrentThread(), 1);
      }

   reset_timebase();

   t_first = t_last = GetTickCount();       

   relative_time = 0;
   last_result   = 0;

   memset(logfile_name, 0, sizeof(logfile_name));
   memset(log_output_string, 0, sizeof(log_output_string));
   log_previous_newline = TRUE;

   smallest_disagreement = 0;
   largest_disagreement  = 0;
   num_disagreements     = 0;
   largest_retrograde    = 0;
   num_retrogrades       = 0;
}

USTIMER::~USTIMER()
{
   if (thread_mask)
      {
      SetThreadAffinityMask(GetCurrentThread(), thread_mask);
      }

   if ((num_disagreements > 0) || (num_retrogrades > 0))
      {
      printf("TIMEUTIL: %d disagreements (%I64d to %I64d), %d retrograde jumps, largest=%I64d\n",
         num_disagreements, 
         smallest_disagreement,
         largest_disagreement,
         num_retrogrades,
         largest_retrograde);
      }

#if TIMEUTIL_THREAD_SAFE
   DeleteCriticalSection(&time_lock);
#endif
}

S64 USTIMER::us(void)
{
   lock();

   //
   // Get # of fine-grained ticks since last call
   //

   S64 hi_res_delta;
   S64 hi_res_result;

   if (QPC_OK)
      {
      //
      // Derive microsecond timer from QueryPerformanceCount() result
      //
   
      S64 q_time;
      QueryPerformanceCounter((LARGE_INTEGER *) &q_time);

      hi_res_result = (S64) (((DOUBLE) (q_time - q_first)) * q_multiplier);
      hi_res_delta  = (S64) (((DOUBLE) (q_time - q_last )) * q_multiplier);
      q_last = q_time;
      }
   else
      {
      S32 m_time = timeGetTime();

      hi_res_result = ((S64) (m_time - m_first)) * 1000LL;
      hi_res_delta  = ((S64) (m_time - m_last))  * 1000LL;
      m_last = m_time;
      }

   //
   // Get # of coarse-grained ticks since last call
   //
   
   S32 t_time = GetTickCount();

   S64 lo_res_result = ((S64) (t_time - t_first)) * 1000LL;
   S64 lo_res_delta  = ((S64) (t_time - t_last))  * 1000LL;
   t_last = t_time;

   //
   // Rebase the high-resolution timer at the current low-resolution tick count 
   // if they disagree by more than 200 ms
   //
   
   S64 d = lo_res_delta - hi_res_delta;    
   
   if ((d < -200000LL) || (d > 200000LL))
      {
      if (d < smallest_disagreement) smallest_disagreement = d;
      if (d > largest_disagreement)  largest_disagreement  = d;
      num_disagreements++;

      relative_time = lo_res_result;
      hi_res_result = 0;
      reset_timebase();
      }
   
   S64 result = hi_res_result + relative_time;

   //
   // Disallow any retrograde jumps
   //
   
   d = result - last_result;

   if (d < 0LL)
      {
      if (d < largest_retrograde) largest_retrograde = d;
      ++num_retrogrades;

      result = last_result;
      }
   else
      {
      last_result = result;
      }

   unlock();
   return result;
}                             

S64 USTIMER::file_time_us (C8 *filename, S64 *creation_time)
{
   union TU
      {
      FILETIME ftime;
      S64      itime;
      };

   TU T,C;

   T.itime = 0;
   C.itime = 0;

   HANDLE infile = CreateFile(filename,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

   if (infile == INVALID_HANDLE_VALUE)
      {
      return -1;
      }

   if (!GetFileTime(infile, 
                   &C.ftime,
                    NULL, 
                   &T.ftime))
      {
      CloseHandle(infile);
      return -1;
      }

   CloseHandle(infile);

   if (creation_time != NULL) 
      {
      *creation_time = C.itime / 10;
      }

   return T.itime / 10;
}

S64 USTIMER::system_time_us (void)
{
   union
      {
      FILETIME ftime;
      S64      itime;
      }
   T;

   T.itime = 0;

   GetSystemTimeAsFileTime(&T.ftime);

   return T.itime / 10;
}

C8 *USTIMER::time_text (S64 time_us, 
                      C8 *text, 
                      S32 text_array_size)
{
   FILETIME   ftime;
   FILETIME   lftime;
   SYSTEMTIME stime;

   assert(text != NULL);

   S64 file_time = time_us * 10;

   ftime.dwLowDateTime = S32(file_time & 0xffffffff);
   ftime.dwHighDateTime = S32(U64(file_time) >> 32);

   FileTimeToLocalFileTime(&ftime,  &lftime);
   FileTimeToSystemTime   (&lftime, &stime);

   memset(text, 0, text_array_size);

   GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
                 0,
                &stime,
                 NULL,
                 text,
                 text_array_size);
   return text;
}

C8 *USTIMER::date_text (S64  time_us, 
                      C8  *text, 
                      S32  text_array_size,
                      C8  *format)
{
   FILETIME   ftime;
   FILETIME   lftime;
   SYSTEMTIME stime;

   assert(text != NULL);

   S64 file_time = time_us * 10;

   ftime.dwLowDateTime = S32(file_time & 0xffffffff);
   ftime.dwHighDateTime = S32(U64(file_time) >> 32);

   FileTimeToLocalFileTime(&ftime,  &lftime);
   FileTimeToSystemTime   (&lftime, &stime);

   memset(text, 0, text_array_size);

   GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                 0,
                &stime,
                 format,
                 text,
                 text_array_size);
   return text;
}

C8 *USTIMER::timestamp(C8 *text,
                     S32 text_array_size,
                     S64 at_time_us)
{
   assert(text != NULL);

   if (!at_time_us)
      {
      at_time_us = system_time_us();
      }

   memset(text, 0, text_array_size);

   C8 d[1024];
   C8 t[1024];

   _snprintf(text,
             text_array_size-1,
             "%s %s",
             date_text(at_time_us, d, sizeof(d)),
             time_text(at_time_us, t, sizeof(t)));

   return text;
}

C8 *USTIMER::duration_string(S64  us, 
                           C8  *text,
                           S32  text_array_size,
                           bool show_msec,
                           bool show_mins)
{
   assert(text != NULL);

   memset(text, 0, text_array_size);
   S32 n = text_array_size - 1;

   C8 *dest = text;

   if (us < 0)
      {
      us = -us;
      strcpy(text,"-");
      dest += strlen(text);
      }

   S64 secs = (us + 500000LL) / 1000000LL;

   S64 years = secs / SECONDS_PER_YEAR;
   secs %= SECONDS_PER_YEAR;

   S64 days =  secs / SECONDS_PER_DAY;
   secs %= SECONDS_PER_DAY;

   S64 hours = secs / SECONDS_PER_HOUR;
   secs %= SECONDS_PER_HOUR;

   S64 mins =  secs / SECONDS_PER_MINUTE;
   secs %= SECONDS_PER_MINUTE;

   if (!show_mins)
      {
      if (years > 0)
         {
         _snprintf(dest, n, "%I64dy %I64dd %I64dh", years, days, hours);
         }
      else if (days > 0)
         {
         _snprintf(dest, n, "%I64dd %I64dh", days, hours);
         }
      else
         {
         _snprintf(dest, n, "%I64dh", hours);
         }
      }
   else
      {
      if (years > 0)
         {
         _snprintf(dest, n, "%I64dy %I64dd %I64dh %I64dm %I64ds", years, days, hours, mins, secs);
         }
      else if (days > 0)
         {
         _snprintf(dest, n, "%I64dd %I64dh %I64dm %I64ds", days, hours, mins, secs);
         }
      else if (hours > 0)
         {
         if ((mins == 0) && (secs == 0))
            _snprintf(dest, n, "%I64dh", hours);
         else
            _snprintf(dest, n, "%I64dh %I64dm %I64ds", hours, mins, secs);
         }
      else if (mins > 0)
         {
         _snprintf(dest, n, "%I64dm %I64ds", mins, secs);
         }
      else if ((us > 1000000LL) || (!show_msec))
         {
         if (!show_msec)
            _snprintf(dest, n, "%I64d s", secs);
         else
            _snprintf(dest, n, "%2.1lf s", (DOUBLE) us / 1000000.0);
         }
      else
         {
         if (us == 0LL)
            _snprintf(dest, n, "0 s");
         else if (us > 1000LL)
            _snprintf(dest, n, "%3.3lf ms", (DOUBLE) us / 1000.0);
         else
            _snprintf(dest, n, "%I64d us", us);
         }
      }

   return text;
}

C8 *USTIMER::JD_timestamp(C8    *text,
                        S32    text_array_size,
                        DOUBLE JD_time)
{
   memset(text, 0, text_array_size);

   if ((JD_time < 0.0) || (JD_time > 4E6))
      {
      return text;
      }

   DOUBLE i, j, k, l, n;

   l = JD_time + 68569.0;
   n = S32(4.0 * l / 146097.0);
   l = l - S32 ((146097.0 * n + 3.0) / 4.0);
   i = S32(4000.0 * (l + 1.0) / 1461001.0);
   l = l - S32(1461.0 * i / 4.0) + 31.0;
   j = S32(80.0 * l / 2447.0);
   k = l - S32(2447.0 * j / 80.0);
   l = S32(j / 11.0);
   j = j + 2.0 - (12.0 * l);
   i = 100.0 * (n - 49.0) + i + l;

   S32 year  = S32(i);
   S32 month = S32(j) % 13;
   S32 day   = S32(k);

   S64 secs = (S64) (((DOUBLE) k - day) * SECONDS_PER_DAY);

   S64 hours = secs / SECONDS_PER_HOUR;
   secs %= SECONDS_PER_HOUR;

   S64 mins =  secs / SECONDS_PER_MINUTE;
   secs %= SECONDS_PER_MINUTE;

   C8 *TOD = (hours >= 12LL) ? "PM" : "AM";

   if (hours == 0)   hours  = 12LL;
   if (hours > 12LL) hours -= 12LL;

   const C8 *mons = "   JanFebMarAprMayJunJulAugSepOctNovDec";

   _snprintf(text,
             text_array_size-1,
             "%d-%c%c%c-%d %I64d:%.02I64d:%.02I64d %s",
             day,
             mons[month*3],mons[month*3+1],mons[month*3+2],
             year,
             hours,
             mins,
             secs,
             TOD);
             
   return text;
}

S64 USTIMER::MJD_to_us(DOUBLE MJD_time)
{
   DOUBLE secs_since_1970 = (MJD_time - MJD_TIME_1970) * SECONDS_PER_DAY;
   return (S64) ((secs_since_1970 * 1E6) + (SYS_TIME_1970 * 1000000LL));
}

DOUBLE USTIMER::us_to_MJD(S64 us)
{
   DOUBLE secs_since_1970 = ((DOUBLE) us / 1E6) - SYS_TIME_1970;
   return (secs_since_1970 / SECONDS_PER_DAY) + MJD_TIME_1970;
}

DOUBLE USTIMER::current_MJD(SYSTEMTIME *st)
{
   SYSTEMTIME St;
   GetSystemTime(&St);

   if (st != NULL) *st = St;

   DOUBLE seconds = (((St.wHour * 60) + St.wMinute) * 60) + St.wSecond;
   seconds += St.wMilliseconds / 1E3;

   return date_to_MJD(St.wYear, St.wMonth, St.wDay) + (seconds / 86400.0);
}

DOUBLE USTIMER::date_to_MJD(S32 year, S32 month, S32 day)
{
   return 367 * year
          - 7 * (year + (month + 9) / 12) / 4
          - 3 * ((year + (month - 9) / 7) / 100 + 1) / 4
          + 275 * month / 9
          + day
          + 1721028
          - 2400000;
}              

S64 USTIMER::NTP_to_us(void *NTP_time)
{
   U8 *ptr = (U8 *) NTP_time;

   U64 secs = (((U32) ptr[0]) << 24) |
              (((U32) ptr[1]) << 16) |
              (((U32) ptr[2]) << 8)  |
               ((U32) ptr[3]);

   U64 frac_secs = (((U32) ptr[4]) << 24) | 
                   (((U32) ptr[5]) << 16) | 
                   (((U32) ptr[6]) << 8)  | 
                    ((U32) ptr[7]);         

   S64 usecs_since_1900 = (secs * 1000000LL) + ((frac_secs * 1000000LL) >> 32LL);

   return usecs_since_1900 - ((NTP_TIME_1970 - SYS_TIME_1970) * 1000000LL);
}

void USTIMER::set_log_filename(C8 *filename)
{
   memset(logfile_name, 0, sizeof(logfile_name));

   if (filename != NULL)
      {
      strncpy(logfile_name, filename, sizeof(logfile_name)-1);
      }
}

C8 * USTIMER::log_printf(C8 *fmt, ...)
{
   va_list ap;

   va_start(ap, 
            fmt);

   C8 *result = log_vprintf(fmt,
                            ap);
   va_end(ap);

   return result;
}

C8 * USTIMER::log_vprintf(C8 *fmt, va_list ap)
{
   lock();

   C8 work_string[TIMEUTIL_MAX_LOG_ENTRY_STRING];

   if (fmt == NULL)
      {
      strcpy(work_string, "(String missing or too large)\n");
      }
   else
      {
      memset(work_string, 0, sizeof(work_string));
 
      _vsnprintf(work_string, 
                 sizeof(work_string)-1,
                 fmt, 
                 ap);
      }

   memset(log_output_string, 0, sizeof(log_output_string));

   if (!log_previous_newline)
      {
      strcpy(log_output_string, work_string);       
      }
   else
      {
      if (work_string[0] == '\n')                   
         {                                          
         _snprintf(log_output_string, 
                   sizeof(log_output_string)-1,
                  "%s",
                   work_string);
         }
      else
         {
         C8 stamp[64] = "";

         _snprintf(log_output_string, 
                   sizeof(log_output_string)-1,
                  "[%s] %s",
                   timestamp(stamp,sizeof(stamp)),
                   work_string);
         }
      }

   if (logfile_name[0])
      {
      FILE *log = fopen(logfile_name,"a+t");

      if (log != NULL)
         {
         fprintf(log, "%s", log_output_string);
         fclose(log);
         }
      }

   log_previous_newline = (log_output_string[strlen(log_output_string)-1] == '\n');

   unlock();
   return log_output_string;
}

// *****************************************************************************
// SCOPETIME implementation
// *****************************************************************************

SCOPETIME::SCOPETIME(C8 *_name)
{
   if (_name != NULL)
      strcpy(name, _name);
   else 
      name[0] = 0;

   start = timer.us();
}

SCOPETIME::~SCOPETIME()
{
   S64 duration = timer.us() - start;

   if (name[0])
      printf("%s: %I64d us\n",name, duration);
   else
      printf("%I64d us\n",duration);
}

// *****************************************************************************
// SHIFTTIME implementation
// *****************************************************************************

static S64 stime_sum = 0;
static S64 stime_cnt = 0;

SHIFTTIME::SHIFTTIME(C8 *_name)
{
   if (_name != NULL)
      strcpy(name, _name);
   else 
      name[0] = 0;

   start = timer.us();
}

SHIFTTIME::~SHIFTTIME()
{
   S64 duration = timer.us() - start;

   stime_sum += duration;
   stime_cnt++;

   if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
      {
      if (name[0])
         printf("%s: %I64d us (avg=%I64d us)\n",name, duration, stime_sum / stime_cnt);
      else
         printf("%I64d us (avg=%I64d us)\n",duration, stime_sum / stime_cnt);
      }
}
