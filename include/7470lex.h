#ifndef HP7470LEX_H
#define HP7470LEX_H

#include <windows.h>
#include <stdio.h>
#include "typedefs.h"
#include "w32sal.h"

// ----------------------------------------------------------------
// Global vars and routines to parse data from HP7470A plotter
//
// 23-Jan-01 jmiles@pop.net
// ----------------------------------------------------------------

extern C8 *plot_data;
extern C8 *plot_filename;
extern C8 *plot_end;
extern C8 *ptr;
extern S32 MaxX, MinX, MaxY, MinY;

//
// Supported plotter commands 
//
// Source: HP7470A Interfacing and Programming Manual, p/n 07470-90001, 
// microfiche no. 07470-90051, June 1982
//

enum COMMAND
{
   CMD_CA,           // Designate Alternate Character Set
   CMD_CP,           // Character Plot
   CMD_CS,           // Designate Standard Character Set
   CMD_DC,           // Digitize Clear
   CMD_DF,           // Default
   CMD_DI,           // Absolute Direction
   CMD_DP,           // Digitize Point
   CMD_DR,           // Relative Direction
   CMD_DT,           // Define Terminator for labels
   CMD_IM,           // Input Mask
   CMD_IN,           // Initialize
   CMD_IP,           // Input P1 and P2
   CMD_IW,           // Input Window
   CMD_LB,           // Label
   CMD_LT,           // Line Type
   CMD_OA,           // Output Actual Position and Pen Status
   CMD_OC,           // Output Commanded Position and Pen Status
   CMD_OD,           // Output Digitized Point and Pen Status
   CMD_OE,           // Output Error
   CMD_OF,           // Output Factors
   CMD_OI,           // Output Identification
   CMD_OO,           // Output Options
   CMD_OP,           // Output P1 and P2
   CMD_OS,           // Output Status
   CMD_OW,           // Output Window
   CMD_PA,           // Plot Absolute
   CMD_PD,           // Pen Down
   CMD_PR,           // Plot Relative
   CMD_PU,           // Pen Up
   CMD_SA,           // Select Alternate Character Set
   CMD_SC,           // Scale
   CMD_SI,           // Absolute Character Size
   CMD_SL,           // Character Slant
   CMD_SM,           // Symbol Mode
   CMD_SP,           // Pen Select
   CMD_SR,           // Relative Character Size
   CMD_SS,           // Select Standard Character Set
   CMD_TL,           // Tick Length
   CMD_UC,           // User Defined Character
   CMD_VS,           // Velocity Select
   CMD_XT,           // X-Tick
   CMD_YT,           // Y-Tick
   CMD_CI,           // Circle
   CMD_AA,           // Arc Absolute
   CMD_AR,           // Arc Relative
   CMD_UNKNOWN,
   CMD_END_OF_DATA,
};

extern C8 *COMMAND_names[CMD_END_OF_DATA];

#include "gpib_utils.h"

S32 next_number(void);
S32 next_comma(void);
COMMAND next_command(void);
COMMAND fetch_command(void);
void fetch_comma(void);
S32 swallow_character(C8 chr);
S32 fetch_integer(void);
SINGLE fetch_float(void);
C8 *fetch_text(S32 terminator);

void * FILE_read (C8     *filename,     
                  S32    *len_dest = NULL,
                  void   *dest     = NULL,
                  S32     len      = 0xffffffff,
                  S32     start_offset = 0);

#endif
