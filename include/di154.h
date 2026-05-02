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
#include "comport.h"
// *****************************************************************************
//
// Read data from DataQ DI-154RS/DI-194RS
// john@miles.io 18-Mar-10
//
// (Large) portions copied from work of Paul Hubbard et al. 
// (http://users.sdsc.edu/~hubbard/neesgrid/dataq/)
//
// *****************************************************************************

//
// Configuration
//

const S32 DI154_MAX_CHANS = 4;

struct DI154_READING
{
    DOUBLE analog[DI154_MAX_CHANS];
    U16    raw   [DI154_MAX_CHANS];
    bool   digital[3];
    bool   is_valid;
};

enum DI154MSGLVL
{
   DI154_DEBUG = 0,  // Debugging traffic
   DI154_VERBOSE,    // Low-level notice
   DI154_NOTICE,     // Standard notice
   DI154_WARNING,    // Warning (does not imply loss of connection or failure of operation)
   DI154_ERROR       // Error (implies failure of operation, and/or connection loss)
};

class DI154
{
public:
   COMPORT serial;
   S32     precision_bits;
   S32     rate_BPS;
   S32     n_chans;
   S32     sample_bytes;

   DI154();

   virtual ~DI154();

   virtual void message_sink(DI154MSGLVL level,   
                             C8         *text);

   virtual void message_printf(DI154MSGLVL level,
                               C8         *fmt,
                               ...);

   virtual S32 buf_validate(const U8 *buf);

   virtual S32 daq_sync();

   virtual S32 buf_decode(const U8      *buf, 
                          DI154_READING *result, 
                          const S32      num_bits);

   virtual BOOL cmd_send(const U8 *data, S32 len, BOOL report_echo_errors = TRUE);

   virtual S32 stop(void);

   virtual S32 start(void);

   virtual S32 reset(void);

   virtual void close(void);

   virtual BOOL open(C8 *port, S32 Hz, U32 analog_chan_mask, U32 digital_chan_mask);

   virtual S32 read(DI154_READING *data, S32 num_bits = 0);

   virtual DOUBLE scale(DOUBLE val, DOUBLE pos_val, DOUBLE neg_val, DOUBLE pos_fs, DOUBLE neg_fs);
};

