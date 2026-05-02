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
// COMPORT: Lightweight API for serial communications     
//
// john@miles.io 7-May-10
//
// *****************************************************************************

//
// Individual COMPORT objects and methods are multithread safe unless
// COMPORT_THREAD_UNSAFE is defined
//
// (You must define COMPORT_THREAD_UNSAFE if compiling a DLL that may be
// dynamically loaded, as the static TLS buffers won't be allocated in XP)
//

#if COMPORT_THREAD_UNSAFE
   #define COMPORT_THREAD_SAFE 0
   #define COMPORT_STATIC static
#else
   #define COMPORT_THREAD_SAFE 1
   #define COMPORT_STATIC static __declspec(thread)
#endif



class COMPORT
{
   bool   use_DTR;
   S32    last_error_code;
   DCB    dcb;

public:
   HANDLE hSerial;

   COMPORT();

   virtual ~COMPORT();

   virtual bool connected(void);

   //
   // Copy OS-specific error text to buffer and return it
   //

   virtual C8 *error_text(S32 last_error = 0);

   virtual void set_DTR(bool on);

   virtual void set_RTS(bool on);

   virtual void disconnect(void);

   virtual S32 connect(C8  *port,                   // Returns zero on success, else an error code that can be passed to error_text() 
                       S32  rate,
                       bool control_DTR = FALSE,    // Resets certain devices such as Arduinos if enabled
                       bool DTR_state   = TRUE);    

   // ---------------------------------------------
   // Higher-level COM routines
   // ---------------------------------------------

   virtual S32 change_rate(S32 rate_BPS);

   virtual S32 printf(C8 *fmt,
                      ...);

   //
   // Write data
   //
   // Returns zero on success, else an error code that can be passed to error_text()   
   //

   virtual S32 write(const C8 *data, 
                     S32      *written = NULL);

   virtual S32 write(const U8 *data, 
                     S32       len, 
                     S32      *written = NULL);

   //
   // Read arbitrary amount of data into user buffer
   //
   // Returns zero on success, else an error code that can be passed to error_text()   
   //

   virtual S32 read(U8  *buff,  
                    S32 *cnt, 
                    S32  timeout_ms = 1000, 
                    S32  dropout_ms = 100,
                    S32  EOS_char   = -1);

   //
   // Read up to 16K bytes of arbitrary data
   //
   // Returns NULL on failure, or pointer to short buffer 
   //

   virtual U8 *read(S32 *cnt, 
                    S32  timeout_ms = 1000, 
                    S32  dropout_ms = 100);

   //
   // Read ASCII data
   //
   // Always returns valid ASCII string of length < 16K
   //

   virtual C8 *gets(S32 EOS_char   = 10, 
                    S32 timeout_ms = 1000, 
                    S32 dropout_ms = 100);

   //
   // Flush any pending data by issuing repetitive read operations with
   // short timeouts until one of the reads times out or fails, or
   // a total of flush_timeout_ms is spent in the loop
   //

   virtual S32 flush(S32 flush_timeout_ms = 200);

};

