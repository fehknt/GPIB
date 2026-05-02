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
#include "winvfx.h"

#define U8 unsigned char



//
// HPGL and simple PCL file drawer by Mark S. Sims
// (C) Copyright 2008, 2013 by Mark S. Sims - all rights reserved.
// Permission granted for free non-commercial use.
//

#define DEG_TO_RAD (3.14159265358970/180.0)
#define MM_PER_GRID (0.025)
#define GL_DIAG_MM  (sqrt(((GL_XMAX-GL_XMIN)*(GL_XMAX-GL_XMIN))+((GL_YMAX-GL_YMIN)*(GL_YMAX-GL_YMIN)))*MM_PER_GRID)

#define SCREEN_WIDTH  RES_X
#define SCREEN_HEIGHT RES_Y

#define COLS (SCREEN_WIDTH-SCREEN_WIDTH/20)    // display window size
#define ROWS (SCREEN_HEIGHT-SCREEN_HEIGHT/20)

#define SCREEN_SIZE_X SCREEN_WIDTH    // full screen size
#define SCREEN_SIZE_Y SCREEN_HEIGHT

#define LEFT_MARGIN (RES_X/40)  // X_MARGIN //((SCREEN_SIZE_X-COLS)/2)
#define TOP_MARGIN  (RES_Y/24)  // (Y_MARGIN+10) // ((SCREEN_SIZE_Y-ROWS)/2)

#define COLOR U8

extern COLOR color;

extern U8 gl_state;
extern char gl_cmd[3];       // two char HPGL command code
extern U8 gl_saw_comma;     // used to fix HP5372 syntax bug
extern U8 gl_alt_font;      // alternate font selected

extern U8 gl_ct_mode;
#define DEFAULT_CHORD 1.0  // default chord angle (standard says 5 degrees)
extern double gl_chord_angle;// tttt

#define GL_ARGLEN 32
extern char gl_arg1[GL_ARGLEN+1];   // first arg for HPGL command
extern char gl_arg2[GL_ARGLEN+1];   // second arg for HPGL command
extern U8 gl_argptr;
extern int gl_pair;                 // used to identify which arg pair we are parsing

#define QUOTE_CMT  0x80  // skipping CO" comment
#define SAW_SLASH  0x40  // used to handle /* and */ nested comments
#define SAW_STAR   0x20
extern U8 gl_cmt;

extern U8 gl_pen;
extern U8 gl_pendown;
extern U8 uc_pendown;
extern U8 gl_moveabs;

extern U8 gl_dvx;
extern U8 gl_dvy;
extern U8 gl_rotate_chars;
extern U8 gl_rotate_coords;

#define PEN_COUNT 8
extern U8 gl_symbol;
extern U8 gl_pw[PEN_COUNT];    // pen widths (in pixels,  0=1 pixel wide)
extern U8 gl_pwu;              // pen width units type (1=rel, 0=mm)
extern U8 gl_charset;
extern U8 gl_term;
extern U8 gl_term_type;
extern U8 gl_td;           // transparent label text (prints control chars)

extern U8 gl_lt;               // line pattern type code
extern U8 gl_lt_rel;           // relative or absolute pattern type
extern double gl_lt_len;        // line pattern length

extern double gl_newx, gl_newy; // current command argument values
extern U8 gl_rel_cmd;

extern double gl_char_sx;
extern U8 gl_rel_char;

extern double gl_x, gl_y; // current pen posiotion
extern double gl_cr_x, gl_cr_y; // carriage return point

extern double gl_textx, gl_texty; // text char size
extern double gl_spacex, gl_spacey; // text char spacing
extern double gl_extra_x, gl_extra_y; // text char spacing adjust
extern double gl_text_ofsx, gl_text_ofsy; // text char offset tweaks

extern double gl_lo_x, gl_lo_y; // label offset command params
extern int gl_lo_code;                // label offset type code

#define GL_TAB_STOP 8
extern int gl_char_num;               // used to calculate tabs

extern double gl_pos_tick;
extern double gl_neg_tick;

#define ENCODED_POLYLINES      // define this to enable the PE command

// vector char size
extern double VX;
extern double VY;

//
// Video stuff
//
// Most (if not all) of the system dependent code is here
//

void lcd_clear();

#define set_color(x) color = (x)
void gl_set_color(U8 pen);

void invert_colors();

//
// These routines mimic routines from the standard MegaDonkey LCD controller
// library (see mega-donkey.com)
//

#define ROT_CHAR_90      0x01
#define ROT_CHAR_VERT    0x02
#define ROT_CHAR_HORIZ   0x04
#define ROT_STRING_VERT  0x08
#define ROT_STRING_HORIZ 0x10
#define ROT_CHAR   (ROT_CHAR_90 | ROT_CHAR_VERT | ROT_CHAR_HORIZ)
#define ROT_STRING (ROT_STRING_VERT | ROT_STRING_HORIZ)
extern U8 rotate;   /* chars: 0x01..0x04   strings: 0x08=verticl  0x10=right->left or bot->top */

#define set_rotate(x) rotate = (x)

//
// Patterned line support
//

#define MAX_PATTERN_LENGTH 20
extern U8 pattern_list[MAX_PATTERN_LENGTH];  // list of pen down/up pairs
extern U8 pattern_length;            // number of values in the pattern list
extern U8 line_pattern;          // where we are in the list plus 1 (if 0, pattern disabled)
extern int pattern_count;
extern U8 temp_line_pattern;
extern COLOR initial_line_color;
extern COLOR temp_pen_color;

void enable_line_pattern();

void disable_line_pattern();

void pause_line_pattern();

void resume_line_pattern();

void step_line_pattern();

//
// Xiaolin Wu's line antialiasing implementation via Mike Abrash
// www.codeproject.com/Articles/13360/Antialiasing-Wu-Algorithm
//

extern short NumLevels;
extern unsigned short IntensityBits;

void WuDot(int x, int y, unsigned short shade);

void draw_Wu_line(short X0, short Y0, short X1, short Y1);

void dot(int x, int y, U8 color);

void draw_norm_line(int x1, int y1, int x2, int y2);

void draw_line(int x1, int y1, int x2, int y2);

// ------------------------------------------------------------------------
// Line Clipper - using Cohen & Sutherland's 4 bit Outcodes
// Ron Grant 1987
// use clipping extents defined within viewport structure pointed to by pVP
// ------------------------------------------------------------------------

typedef int int88;

typedef struct
{
   int OrgX,OrgY;       // view center in world coordinates
   int Theta;           // viewport rotation with respect to world (0..359)
   int Width,Height;    // viewport width and height in world coordinates
   // OR do we specify scale directly rather than doing
   // indirect calculations to map world viewport to screen viewport

   int88 ScaleX,ScaleY; // fixed point scale

   int   WinX1,WinY1,WinX2,WinY2; // viewport window screen coordinates
   // Note: using unsigned values here caused problems with
   // clipper outcode function, that is, comparison of
   // negative ordinate against window edge was failing --
   // e.g. x=-1  left=10   (x<left) was generating FALSE

   int EyeX,EyeY;
} 
viewport;

extern viewport *pVP;  // pointer to current viewport used by clipper
extern viewport VP;    // Default Instance of viewport (more can be defined)

#define select_viewport(v) pVP=&v    /* point to current vieport */

void viewport_init(void);   // sets up default clipping to full screen

#define CS_LEFT   1
#define CS_RIGHT  2
#define CS_BOTTOM 4
#define CS_TOP    8

U8 out_code(int x, int y);  // clipped line endpoint region test

U8 clipped_line(int x1,int y1, int x2,int y2);   // clipped line

void clipped_box(int x1,int y1, int x2,int y2);

void clipped_filled_box(int x1,int y1, int x2,int y2);

//
// Vector Character Table
// this table remains in program memory
// requires special functions to access
//
// Each byte in a stroke list contains a pair of coordinate offsets
// and two flag bits.
//   LINE:COL:EOL:ROW    (MOVE:1 bit  COL:3 bits    EOL:1 bit ROW:3 bits)
// If the LINE bit is 0,  then move to the new coordinate and draw a dot.
// If the LINE bit is 1,  draw a line from the current coordinate to the new
//                        coordinate.
// If the EOL bit is set, the byte is the last one in the stroke list
// A 0xFF byte indicates a null character.
//
// This table includes negative ASCII characters, even though they aren't technically
// supported by the 7470 plotter
//

#define PROGMEM
extern U8 vg_00[] PROGMEM;
extern U8 vg_01[] PROGMEM;
extern U8 vg_02[] PROGMEM;
extern U8 vg_03[] PROGMEM;
extern U8 vg_04[] PROGMEM;
extern U8 vg_05[] PROGMEM;
extern U8 vg_06[] PROGMEM;
extern U8 vg_07[] PROGMEM;
extern U8 vg_08[] PROGMEM;
extern U8 vg_09[] PROGMEM;
extern U8 vg_0A[] PROGMEM;
extern U8 vg_0B[] PROGMEM;
extern U8 vg_0C[] PROGMEM;
extern U8 vg_0D[] PROGMEM;
extern U8 vg_0E[] PROGMEM;
extern U8 vg_0F[] PROGMEM;
extern U8 vg_10[] PROGMEM;
extern U8 vg_11[] PROGMEM;
extern U8 vg_12[] PROGMEM;
extern U8 vg_13[] PROGMEM;
extern U8 vg_14[] PROGMEM;
extern U8 vg_15[] PROGMEM;
extern U8 vg_16[] PROGMEM;
extern U8 vg_17[] PROGMEM;
extern U8 vg_18[] PROGMEM;
extern U8 vg_19[] PROGMEM;
extern U8 vg_1A[] PROGMEM;
extern U8 vg_1B[] PROGMEM;
extern U8 vg_1C[] PROGMEM;
extern U8 vg_1D[] PROGMEM;
extern U8 vg_1E[] PROGMEM;
extern U8 vg_1F[] PROGMEM;
extern U8 vg_20[] PROGMEM;
extern U8 vg_21[] PROGMEM;
extern U8 vg_22[] PROGMEM;
extern U8 vg_23[] PROGMEM;
extern U8 vg_24[] PROGMEM;
extern U8 vg_25[] PROGMEM;
extern U8 vg_26[] PROGMEM;
extern U8 vg_27[] PROGMEM;
extern U8 vg_28[] PROGMEM;
extern U8 vg_29[] PROGMEM;
extern U8 vg_2A[] PROGMEM;
extern U8 vg_2B[] PROGMEM;
extern U8 vg_2C[] PROGMEM;
extern U8 vg_2D[] PROGMEM;
extern U8 vg_2E[] PROGMEM;
extern U8 vg_2F[] PROGMEM;
extern U8 vg_30[] PROGMEM;
extern U8 vg_31[] PROGMEM;
extern U8 vg_32[] PROGMEM;
extern U8 vg_33[] PROGMEM;
extern U8 vg_34[] PROGMEM;
extern U8 vg_35[] PROGMEM;
extern U8 vg_36[] PROGMEM;
extern U8 vg_37[] PROGMEM;
extern U8 vg_38[] PROGMEM;
extern U8 vg_39[] PROGMEM;
extern U8 vg_3A[] PROGMEM;
extern U8 vg_3B[] PROGMEM;
extern U8 vg_3C[] PROGMEM;
extern U8 vg_3D[] PROGMEM;
extern U8 vg_3E[] PROGMEM;
extern U8 vg_3F[] PROGMEM;
extern U8 vg_40[] PROGMEM;
extern U8 vg_41[] PROGMEM;
extern U8 vg_42[] PROGMEM;
extern U8 vg_43[] PROGMEM;
extern U8 vg_44[] PROGMEM;
extern U8 vg_45[] PROGMEM;
extern U8 vg_46[] PROGMEM;
extern U8 vg_47[] PROGMEM;
extern U8 vg_48[] PROGMEM;
extern U8 vg_49[] PROGMEM;
extern U8 vg_4A[] PROGMEM;
extern U8 vg_4B[] PROGMEM;
extern U8 vg_4C[] PROGMEM;
extern U8 vg_4D[] PROGMEM;
extern U8 vg_4E[] PROGMEM;
extern U8 vg_4F[] PROGMEM;
extern U8 vg_50[] PROGMEM;
extern U8 vg_51[] PROGMEM;
extern U8 vg_52[] PROGMEM;
extern U8 vg_53[] PROGMEM;
extern U8 vg_54[] PROGMEM;
extern U8 vg_55[] PROGMEM;
extern U8 vg_56[] PROGMEM;
extern U8 vg_57[] PROGMEM;
extern U8 vg_58[] PROGMEM;
extern U8 vg_59[] PROGMEM;
extern U8 vg_5A[] PROGMEM;
extern U8 vg_5B[] PROGMEM;
extern U8 vg_5C[] PROGMEM;
extern U8 vg_5D[] PROGMEM;
extern U8 vg_5E[] PROGMEM;
extern U8 vg_5F[] PROGMEM;
extern U8 vg_60[] PROGMEM;
extern U8 vg_61[] PROGMEM;
extern U8 vg_62[] PROGMEM;
extern U8 vg_63[] PROGMEM;
extern U8 vg_64[] PROGMEM;
extern U8 vg_65[] PROGMEM;
extern U8 vg_66[] PROGMEM;
extern U8 vg_67[] PROGMEM;
extern U8 vg_68[] PROGMEM;
extern U8 vg_69[] PROGMEM;
extern U8 vg_6A[] PROGMEM;
extern U8 vg_6B[] PROGMEM;
extern U8 vg_6C[] PROGMEM;
extern U8 vg_6D[] PROGMEM;
extern U8 vg_6E[] PROGMEM;
extern U8 vg_6F[] PROGMEM;
extern U8 vg_70[] PROGMEM;
extern U8 vg_71[] PROGMEM;
extern U8 vg_72[] PROGMEM;
extern U8 vg_73[] PROGMEM;
extern U8 vg_74[] PROGMEM;
extern U8 vg_75[] PROGMEM;
extern U8 vg_76[] PROGMEM;
extern U8 vg_77[] PROGMEM;
extern U8 vg_78[] PROGMEM;
extern U8 vg_79[] PROGMEM;
extern U8 vg_7A[] PROGMEM;
extern U8 vg_7B[] PROGMEM;
extern U8 vg_7C[] PROGMEM;
extern U8 vg_7D[] PROGMEM;
extern U8 vg_7E[] PROGMEM;
extern U8 vg_7F[] PROGMEM;
extern U8 vg_80[] PROGMEM;
extern U8 vg_81[] PROGMEM;
extern U8 vg_82[] PROGMEM;
extern U8 vg_83[] PROGMEM;
extern U8 vg_84[] PROGMEM;
extern U8 vg_85[] PROGMEM;
extern U8 vg_86[] PROGMEM;
extern U8 vg_87[] PROGMEM;
extern U8 vg_88[] PROGMEM;
extern U8 vg_89[] PROGMEM;
extern U8 vg_8A[] PROGMEM;
extern U8 vg_8B[] PROGMEM;
extern U8 vg_8C[] PROGMEM;
extern U8 vg_8D[] PROGMEM;
extern U8 vg_8E[] PROGMEM;
extern U8 vg_8F[] PROGMEM;
extern U8 vg_90[] PROGMEM;
extern U8 vg_91[] PROGMEM;
extern U8 vg_92[] PROGMEM;
extern U8 vg_93[] PROGMEM;
extern U8 vg_94[] PROGMEM;
extern U8 vg_95[] PROGMEM;
extern U8 vg_96[] PROGMEM;
extern U8 vg_97[] PROGMEM;
extern U8 vg_98[] PROGMEM;
extern U8 vg_99[] PROGMEM;
extern U8 vg_9A[] PROGMEM;
extern U8 vg_9B[] PROGMEM;
extern U8 vg_9C[] PROGMEM;
extern U8 vg_9D[] PROGMEM;
extern U8 vg_9E[] PROGMEM;
extern U8 vg_9F[] PROGMEM;
extern U8 vg_A0[] PROGMEM;
extern U8 vg_A1[] PROGMEM;
extern U8 vg_A2[] PROGMEM;
extern U8 vg_A3[] PROGMEM;
extern U8 vg_A4[] PROGMEM;
extern U8 vg_A5[] PROGMEM;
extern U8 vg_A6[] PROGMEM;
extern U8 vg_A7[] PROGMEM;
extern U8 vg_A8[] PROGMEM;
extern U8 vg_A9[] PROGMEM;
extern U8 vg_AA[] PROGMEM;
extern U8 vg_AB[] PROGMEM;
extern U8 vg_AC[] PROGMEM;
extern U8 vg_AD[] PROGMEM;
extern U8 vg_AE[] PROGMEM;
extern U8 vg_AF[] PROGMEM;
extern U8 vg_B0[] PROGMEM;
extern U8 vg_B1[] PROGMEM;
extern U8 vg_B2[] PROGMEM;
extern U8 vg_B3[] PROGMEM;
extern U8 vg_B4[] PROGMEM;
extern U8 vg_B5[] PROGMEM;
extern U8 vg_B6[] PROGMEM;
extern U8 vg_B7[] PROGMEM;
extern U8 vg_B8[] PROGMEM;
extern U8 vg_B9[] PROGMEM;
extern U8 vg_BA[] PROGMEM;
extern U8 vg_BB[] PROGMEM;
extern U8 vg_BC[] PROGMEM;
extern U8 vg_BD[] PROGMEM;
extern U8 vg_BE[] PROGMEM;
extern U8 vg_BF[] PROGMEM;
extern U8 vg_C0[] PROGMEM;
extern U8 vg_C1[] PROGMEM;
extern U8 vg_C2[] PROGMEM;
extern U8 vg_C3[] PROGMEM;
extern U8 vg_C4[] PROGMEM;
extern U8 vg_C5[] PROGMEM;
extern U8 vg_C6[] PROGMEM;
extern U8 vg_C7[] PROGMEM;
extern U8 vg_C8[] PROGMEM;
extern U8 vg_C9[] PROGMEM;
extern U8 vg_CA[] PROGMEM;
extern U8 vg_CB[] PROGMEM;
extern U8 vg_CC[] PROGMEM;
extern U8 vg_CD[] PROGMEM;
extern U8 vg_CE[] PROGMEM;
extern U8 vg_CF[] PROGMEM;
extern U8 vg_D0[] PROGMEM;
extern U8 vg_D1[] PROGMEM;
extern U8 vg_D2[] PROGMEM;
extern U8 vg_D3[] PROGMEM;
extern U8 vg_D4[] PROGMEM;
extern U8 vg_D5[] PROGMEM;
extern U8 vg_D6[] PROGMEM;
extern U8 vg_D7[] PROGMEM;
extern U8 vg_D8[] PROGMEM;
extern U8 vg_D9[] PROGMEM;
extern U8 vg_DA[] PROGMEM;
extern U8 vg_DB[] PROGMEM;
extern U8 vg_DC[] PROGMEM;
extern U8 vg_DD[] PROGMEM;
extern U8 vg_DE[] PROGMEM;
extern U8 vg_DF[] PROGMEM;
extern U8 vg_E0[] PROGMEM;
extern U8 vg_E1[] PROGMEM;
extern U8 vg_E2[] PROGMEM;
extern U8 vg_E3[] PROGMEM;
extern U8 vg_E4[] PROGMEM;
extern U8 vg_E5[] PROGMEM;
extern U8 vg_E6[] PROGMEM;
extern U8 vg_E7[] PROGMEM;
extern U8 vg_E8[] PROGMEM;
extern U8 vg_E9[] PROGMEM;
extern U8 vg_EA[] PROGMEM;
extern U8 vg_EB[] PROGMEM;
extern U8 vg_EC[] PROGMEM;
extern U8 vg_ED[] PROGMEM;
extern U8 vg_EE[] PROGMEM;
extern U8 vg_EF[] PROGMEM;
extern U8 vg_F0[] PROGMEM;
extern U8 vg_F1[] PROGMEM;
extern U8 vg_F2[] PROGMEM;
extern U8 vg_F3[] PROGMEM;
extern U8 vg_F4[] PROGMEM;
extern U8 vg_F5[] PROGMEM;
extern U8 vg_F6[] PROGMEM;
extern U8 vg_F7[] PROGMEM;
extern U8 vg_F8[] PROGMEM;
extern U8 vg_F9[] PROGMEM;
extern U8 vg_FA[] PROGMEM;
extern U8 vg_FB[] PROGMEM;
extern U8 vg_FC[] PROGMEM;
extern U8 vg_FD[] PROGMEM;
extern U8 vg_FE[] PROGMEM;
extern U8 vg_FF[] PROGMEM;

//
// Table of pointers to the character strokes
//

extern U8 *vgen[256];

#define VCHAR_W  8 // elemental width and height of character pattern 
#define VCHAR_H  8
extern U8 vchar_inited;

// vector character attributes
// set generally by function calls

extern signed char VCharScaleX;
extern signed char VCharScaleY;

extern int VCharHeight;     // character height computed
extern int VCharWidth;      // character width  computed

extern int VCharTable;
extern int VCharThicknessX;
extern int VCharThicknessY;

extern int VCharSpaceX;
extern int VCharSpaceY;

void vchar_init(void);

void vchar_set_fontsize(U8 scaleX, U8 scaleY);

void vchar_set_thickness(U8 thick_x, U8 thick_y);

void vchar_set_spacing(U8 space_x, U8 space_y);

extern signed char vchar_slant[VCHAR_H];

#define DL_LEN  256                 // uuuu max number of strokes in a DL char
#define UC_CHAR 256                 // UC char is DL font char 256
extern int gl_dl_font[2][256+1][DL_LEN+4]; // DL download font buffer
extern int gl_uc_ptr;                      // used to save UC use character strokes
void draw_user_char(double x,double y, int font, int set_xy, int c);

void vchar_char(int xoffset,int yoffset, int c);  // draw a vector character

extern double CurX,CurY; // Current X,Y point as entity drawing progresses

extern double u, du, X1, Y1, X2, Y2, X3, Y3, X4, Y4;

extern U8 DrawCmdActive;

#define DC_BEZ 5

void bezier_init(long steps);

// next_u: advance to next u value, if not right at end
// if u beyond end make = to end such that next call to nextpoint
// routine will gen point right at end of curve or line.

void next_u(void);

void bezier_nextpoint(void);

//
// User interface routines
//

extern U8 new_lbterm;

extern int pcl_row, pcl_col; // where to draw the scan line data
extern int pcl_encoding;
extern U8 pcl_state;
extern int pcl_rep_count;

void gl_set_defaults(U8 init);

void render_init();

#ifdef PCL_CODE
//
//
//   This is a PCL command parser.
//
//   It has some crude support for rendering uncompressed raster
//   graphics (just enough to render HP16500 and HP5371 screen prints).
//
//   There is some attempt to scale graphics to a (smaller) screen.
//   Horizontally the raster is scaled to the screen width by converting
//   runs of pixels to vectors and then scaling the vectors to the screen size.
//   Vertically the raster is scaled by dropping rows.
//
//
extern U8 pcl_next_state; // ... state to enter after processing a PCL command

extern U8 pcl_parm;       // the PCL parm command
extern U8 pcl_grp;        // the PCL group command
extern long pcl_value;

extern int pcl_sign;
extern U8 pcl_decimal;
extern long pcl_divisor;

#define PCL_SCALE 1
#define PCL_SCALE_VECTORS 1
extern int pcl_bytes;
extern int pcl_run_start;
extern U8 pcl_last_bit;

#define PCL_BUF_LEN 4096    // good for 32768 pixels
extern U8 pcl_buf[PCL_BUF_LEN+1];
extern int pcl_index;


void pcl_bit(U8 val);

void pcl_byte(U8 data);

void pcl_rep(int i, U8 data);

void pcl_reset(U8 flag);

void do_pcl_cmd(U8 pcl_cmd);

void parse_pcl(U8 data);
#endif

//
// HPGL renderer
//
// Coordinate scaling and screen mapping routines
//

int scale_x(double x);


int scale_y(double y);

double unscale_x(int x);

double unscale_y(int y);

double rotate_x(double x, double y);

double rotate_y(double x, double y);

void gl_clip_window(U8 flag);
