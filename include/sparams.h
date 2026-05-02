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
#include "spline.h"
/*********************************************************************/
//
// S-parameter storage class, including .SNP file access, serialization,
// interpolation, and caching
//
// john@miles.io
//  
/*********************************************************************/

namespace SNPTYPE       // Flags used to indicate which format(s) are cached in database
{
   const U8 MA = 0x01;  // Magnitude-angle form is valid
   const U8 DB = 0x02;  // dB-angle form is valid
   const U8 RI = 0x04;  // Real-imag form is valid
   const U8 CZ = 0x08;  // Complex impedance (R+jX) is valid (conversion based on real part of Zo)
}

namespace SPARAM
{
   struct MA 
      { 
      DOUBLE mag; 
      DOUBLE deg; 
      
      MA(DOUBLE m, DOUBLE a); 
      MA(struct DB); 
      MA(struct RI); 
      };

   struct DB 
      { 
      DOUBLE dB;  
      DOUBLE deg; 

      DB(DOUBLE d, DOUBLE a); 
      DB(struct MA);
      DB(struct RI);
      };

   struct RI : public COMPLEX_DOUBLE 
      { 
      RI(DOUBLE r, DOUBLE i);
      RI(COMPLEX_DOUBLE c);
      RI(struct MA);
      RI(struct DB);
      };

   struct CZ 
      { 
      DOUBLE R;   
      DOUBLE jX;  
      
      CZ(DOUBLE r, DOUBLE x); 
      CZ(struct MA, DOUBLE Ro);
      };

   enum MSGLVL
      {
      MSG_DEBUG = 0,    // Debugging traffic
      MSG_VERBOSE,      // Low-level notice
      MSG_NOTICE,       // Standard notice
      MSG_WARNING,      // Warning
      MSG_ERROR         // Error
      };

   const U8 EXT_ZERO          = 0x01;        // Frequency-based queries outside min/max range return valid zero magnitude and phase
   const U8 EXT_LEND          = 0x02;        // Frequency-based queries below min return valid min endpoint
   const U8 EXT_REND          = 0x04;        // Frequency-based queries above max return valid max endpoint
   const U8 EXT_ENDS          = EXT_LEND | EXT_REND;

   const U32 BIN_ID           = 'BPNS';      // Binary stream identifier 'SNPB' (little-endian)
   const U32 BIN_VERSION      = 0x00000001;  // Binary stream version written by this implementation

   extern const C8 *DEF_DATA_FORMAT;        // Default format for .S2P file writes
   extern const C8 *DEF_FREQ_FORMAT;
}

struct SPARAMS
{
   C8             message_text[4096];     // Error/warning text buffer for optional app access

   S32            n_ports;                // Matrix dimensions S[m][m], currently must be either 1 or 2
   S32            n_points; 

   DOUBLE         min_Hz;                 // Valid after read_SNP_file() or application-specific setup
   DOUBLE         max_Hz;                 // Application can access these variables (and the arrays below) directly
   COMPLEX_DOUBLE Zo;

   DOUBLE        *freq_Hz;                // [n_points]
   U8          ***valid;                  // [b][a][n_points]
   SPARAM::MA  ***MA;                     // [b][a][n_points]
   SPARAM::DB  ***DB;
   SPARAM::RI  ***RI;
   SPARAM::CZ  ***CZ;

   // --------------------------------------------------------------------------------------------------
   // Error/status message sink can be subclassed if desired
   // to redirect output
   // --------------------------------------------------------------------------------------------------

   virtual void message_sink(SPARAM::MSGLVL level,   
                             C8            *text);

   virtual void message_printf(SPARAM::MSGLVL level,
                               C8            *fmt,
                               ...);

   // --------------------------------------------------------------------------------------------------
   // Construction/destruction
   // --------------------------------------------------------------------------------------------------

   SPARAMS();

   virtual ~SPARAMS();

   // --------------------------------------------------------------------------------------------------
   // Set construction defaults
   // --------------------------------------------------------------------------------------------------

   virtual void init(void);

   // --------------------------------------------------------------------------------------------------
   // Discard existing database
   // --------------------------------------------------------------------------------------------------

   virtual void clear(void);

   // --------------------------------------------------------------------------------------------------
   // Reserve specified number of ports (typically 1 or 2) and data points
   // --------------------------------------------------------------------------------------------------

   virtual bool alloc(S32 ports, S32 points);

   // --------------------------------------------------------------------------------------------------
   // Serialize to binary block
   //
   //   Header: U8  identifier[4] 'SNPB'
   //           U32 version
   //           S32 n_data_bytes (not including header)
   //
   // Contents: ...
   //           Version-specific data
   //           ...
   //
   // Caller must free the returned block
   // --------------------------------------------------------------------------------------------------

   virtual U8 *serialize(S32 *output_bytes);

   // --------------------------------------------------------------------------------------------------
   // Deserialize from open binary file handle
   //
   // Returns # of bytes processed or -1 on error
   // (0 = file contents not recognized as a serialized .S2P file)
   // --------------------------------------------------------------------------------------------------

   virtual S32 deserialize(FILE *in);

   // --------------------------------------------------------------------------------------------------
   // Deserialize from memory block
   //
   // Returns # of bytes processed or -1 on error
   // (0 = data not recognized as a serialized .S2P file)
   // --------------------------------------------------------------------------------------------------

   virtual S32 deserialize(U8 *block);

   // --------------------------------------------------------------------------------------------------
   // Remove non-Touchstone compatible characters from string
   // --------------------------------------------------------------------------------------------------

   virtual C8 *sanitize(const C8 *input);

   // --------------------------------------------------------------------------------------------------
   // Find data point closest to the specified frequency, or -1 if out of range
   //
   // alpha=0.0 if point matches freq_Hz[result] (or is an endpoint), 
   // 1.0 if point matches freq_Hz[result+1]
   // --------------------------------------------------------------------------------------------------

   static int search_double_array(const void *keyval, const void *datum);

   S32 nearest_freq_Hz(DOUBLE Hz, DOUBLE *alpha = NULL);

   // --------------------------------------------------------------------------------------------------
   // Return TRUE if value at specified point is available in any format
   // --------------------------------------------------------------------------------------------------

   virtual bool point_valid(S32 pt, S32 param);

   // --------------------------------------------------------------------------------------------------
   // Write accessors
   // --------------------------------------------------------------------------------------------------

   virtual void set_RI(S32 pt, S32 param, COMPLEX_DOUBLE val);

   virtual void set_RI(S32 pt, S32 b, S32 a, COMPLEX_DOUBLE val);

   // --------------------------------------------------------------------------------------------------
   // Accessor routines return valid S-parameter value in desired format
   //
   // Parameters can be requested by index [b,a] or by position in Touchstone file 0-3
   // (0=S11, 1=S21, 2=S12, 3=S22)
   //
   // Attempting to request a value that has never been written in any format 
   // triggers an assert
   // --------------------------------------------------------------------------------------------------

   virtual SPARAM::RI get_RI(S32 pt, S32 param);

   virtual void get_RI(S32 pt);

   virtual SPARAM::RI get_RI(S32 pt, S32 b, S32 a);

   virtual SPARAM::MA get_MA(S32 pt, S32 param);

   virtual void get_MA(S32 pt);

   virtual SPARAM::MA get_MA(S32 pt, S32 b, S32 a);

   virtual void get_DB(S32 pt);

   virtual SPARAM::DB get_DB(S32 pt, S32 param);

   virtual SPARAM::DB get_DB(S32 pt, S32 b, S32 a);

   virtual SPARAM::CZ get_CZ(S32 pt, S32 param);

   virtual void get_CZ(S32 pt);

   virtual SPARAM::CZ get_CZ(S32 pt, S32 b, S32 a);

   // --------------------------------------------------------------------------------------------------
   // Frequency-based queries return S-parameter value in desired format, interpolated
   // to specified frequency
   //
   //   RI: Interpolate cartesian I and Q independently
   //   MA: Interpolate cartesian magnitude and polar angle, returning phase in [-PI,PI]
   //   DB: Same as MA with interpolated magnitude converted back to DB
   //   CZ: Same as MA with interpolated value converted back to CZ
   //
   // Phase/magnitude values beyond min/max frequencies are extrapolated as specified by flags:
   //
   //   EXT_ZERO: Return valid zero value for all out-of-range queries 
   //   EXT_LEND: Return valid copy of leftmost endpoint for queries below min freq 
   //   EXT_REND: Return valid copy of rightmost endpoint for queries above max freq
   // --------------------------------------------------------------------------------------------------

   virtual SPARAM::RI get_RI(DOUBLE Hz, S32 b, S32 a, U8 flags, bool *in_range);

   virtual SPARAM::MA get_MA(DOUBLE Hz, S32 b, S32 a, U8 flags, bool *in_range);

   virtual SPARAM::DB get_DB(DOUBLE Hz, S32 b, S32 a, U8 flags, bool *in_range);

   virtual SPARAM::CZ get_CZ(DOUBLE Hz, S32 b, S32 a, U8 flags, bool *in_range);

   // --------------------------------------------------------------------------------------------------
   // Perform T-Check calibration assessment
   // --------------------------------------------------------------------------------------------------

   virtual bool T_check(DOUBLE *out);

   // --------------------------------------------------------------------------------------------------
   // Save data to Touchstone 1.1 file
   // --------------------------------------------------------------------------------------------------

   virtual bool write_SNP_file(const C8 *filename,   
                               const C8 *data_format       = SPARAM::DEF_DATA_FORMAT, // e.g., "MA",       
                               const C8 *freq_format       = SPARAM::DEF_FREQ_FORMAT, // e.g., "GHZ",
                               const C8 *header            = NULL,                    // optional
                               const C8 *single_param_type = NULL);                   // optional 

   // --------------------------------------------------------------------------------------------------
   // Load contents of Touchstone 1.1 file (e.g., .s1p, .s2p) 
   //
   // NB: There's no straightforward way to tell how many ports are specified in a 
   // Touchstone 1.X file, so the target database size must be specified in file_ports
   // --------------------------------------------------------------------------------------------------

   virtual bool read_SNP_file(const C8 *filename, 
                              S32       file_ports);

   // --------------------------------------------------------------------------------------------------
   // Utility functions to generate interpolated frequency array compatible with
   // the spline and linear interpolators below
   //
   // Optionally return the first and last valid SNP indexes
   // --------------------------------------------------------------------------------------------------

   virtual void interp_Hz(DOUBLE  out_min_Hz,
                          DOUBLE  out_max_Hz,
                          S32     n_out_points,
                          DOUBLE *out_Hz = NULL,
                          S32    *p0     = NULL,
                          S32    *p1     = NULL);

   static void interp_Hz(DOUBLE  out_min_Hz,
                         DOUBLE  out_max_Hz,
                         DOUBLE *out_Hz,
                         S32     n_out_points);

   virtual void preset_Hz_range(void);

   // ---------------------------------
   // Spline interpolators 
   // ---------------------------------

   virtual void spline_dB(S32     b,
                          S32     a, 
                          DOUBLE  out_min_Hz, 
                          DOUBLE  out_max_Hz, 
                          S32     n_out_points,
                          DOUBLE *out_dB,
                          DOUBLE *out_Hz = NULL);

   virtual void spline_deg(S32     b,
                           S32     a,
                           DOUBLE  out_min_Hz,       
                           DOUBLE  out_max_Hz,       
                           S32     n_out_points,
                           DOUBLE *out_deg,        
                           DOUBLE *out_Hz = NULL);

   // ---------------------------------
   // Linear interpolators
   // ---------------------------------

   virtual void lerp_dB(S32     b,
                        S32     a,
                        DOUBLE  out_min_Hz, 
                        DOUBLE  out_max_Hz, 
                        S32     n_out_points,
                        DOUBLE *out_dB,
                        DOUBLE *out_Hz,
                        bool   *out_valid,
                        U8      flags);

   virtual void lerp_deg(S32     b,
                         S32     a,
                         DOUBLE  out_min_Hz,       
                         DOUBLE  out_max_Hz,       
                         S32     n_out_points,
                         DOUBLE *out_deg,        
                         DOUBLE *out_Hz,
                         bool   *out_valid,
                         U8      flags);
};

