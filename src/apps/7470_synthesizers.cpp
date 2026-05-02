#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
#include <float.h>

#include "typedefs.h"
#include "gpiblib.h"
#include "specan.h"
#include "8566plt.h"
#include "49xplt.h"
#include "w32sal.h"
#include "7470_synthesizers.h"

// Externs from 7470.cpp or other modules
extern void __cdecl GPIB_print(C8 *fmt, ...);
extern S32 INI_ignore_write_aborts;

// Global buffers moved from 7470.cpp or shared
C8 synthesis_buffer[4194304];
C8 trace_buffer    [4194304];

// Helper for 492P synthesis
enum FT_LSD_MODE
{
   FT_LSD_E3,
   FT_LSD_HZ,
   FT_LSD_MHZ,
   FT_LSD_INT
};

static C8 *freq_text(DOUBLE f, FT_LSD_MODE LSD)
{
   static C8 text[256];

   DOUBLE a = fabs(f);

   if (LSD == FT_LSD_INT)
      {
           if (a >= 1E9) f /= 1E9;                 
      else if (a >= 1E6) f /= 1E6;                        
      else if (a >= 1E3) f /= 1E3;                        

      S32 d = (S32) ((f >= 0) ? (f + 0.5) : (f - 0.5));

      if (a >= 1E9)
         sprintf(text, "%d GHz", d);
      else if (a >= 1E6)
         sprintf(text, "%d MHz", d);
      else if (a >= 1E3)
         sprintf(text, "%d kHz", d);
      else if ((a > 0) && (a < 1.0))
         sprintf(text,"%0.1f Hz", f);       
      else
         sprintf(text,"%d Hz", d);
      }
   else if (LSD == FT_LSD_MHZ)
      {
      if (a >= 1E9)  
         {
         f /= 1E9;                 
         sprintf(text, "%0.3lf GHz", f);   // display with LSD rounded to nearest MHz
         }
      else
         {
         f /= 1E6;                        
         S32 d = (S32) ((f >= 0) ? (f + 0.5) : (f - 0.5));
         sprintf(text, "%d MHz", d);       // display integer MHz
         }
      }
   else if (LSD == FT_LSD_E3)
      {
      if (a >= 1E9)
         sprintf(text, "%0.3lf GHz", f / 1E9);   // display with LSD rounded to nearest MHz
      else if (a >= 1E6)
         sprintf(text, "%0.3lf MHz", f / 1E6);   // display with LSD rounded to nearest kHz
      else if (a >= 1E3)
         sprintf(text, "%0.3lf kHz", f / 1E3);   // display with LSD rounded to nearest Hz
      else
         sprintf(text,"%0.lf Hz", f);            // display with LSD rounded to nearest Hz 
      }
   else if (LSD == FT_LSD_HZ)
      {
      if (a >= 1E9)
         sprintf(text, "%0.9lf GHz", f / 1E9);   // display with LSD rounded to nearest Hz
      else if (a >= 1E6)
         sprintf(text, "%0.6lf MHz", f / 1E6);   // display with LSD rounded to nearest Hz
      else if (a >= 1E3)
         sprintf(text, "%0.3lf kHz", f / 1E3);   // display with LSD rounded to nearest Hz
      else
         sprintf(text,"%0.lf Hz", f);            // display with LSD rounded to nearest Hz 
      }

   return text;
}

// Macros for Tek 49x and HP 8566 scaling
#define X_GRAT_49X 200
#define Y_GRAT_49X 100
#define W_GRAT_49X 2000
#define H_GRAT_49X 800

static S32 x_to_49x_scale(SA_STATE *SA, S32 n)   // n=[0,W_GRAT)
{
   S32 x = X_GRAT_49X + n;

   if (x >= X_GRAT_49X + W_GRAT_49X) x = X_GRAT_49X + W_GRAT_49X - 1;
   if (x <  X_GRAT_49X)          x = X_GRAT_49X;

   return x;
}

static S32 y_to_49x_scale(SA_STATE *SA, DOUBLE dBm) 
{
   S32 y = Y_GRAT_49X + (S32) (((dBm - SA->min_dBm) * (F32) H_GRAT_49X) / (SA->max_dBm - SA->min_dBm));

   if (y >= Y_GRAT_49X + H_GRAT_49X) y = Y_GRAT_49X + H_GRAT_49X - 1;
   if (y <  Y_GRAT_49X)          y = Y_GRAT_49X;

   return y;
}

static void write_49x_field(C8 *buffer, const C8 *tag, const C8 *src, const C8 *suffix)
{
   C8 *txt = strstr(buffer, tag);
   assert(txt != NULL);

   for (S32 i=0; i < (S32) strlen(tag); i++)
      {
      txt[i] = ' ';
      }

   C8 string[256];
   strcpy(string, src);
   strcat(string, suffix);

   for (S32 i=0; i < (S32) strlen(string); i++)
      {
      if (txt[i] != ' ') break;
      txt[i] = string[i];
      }
}

static void write_49x_int(C8 *buffer, const C8 *tag, DOUBLE val, const C8 *suffix)
{
   S32 n = 0;

   if (val >= 0.0F)
      n = (S32) (val + 0.5);
   else
      n = (S32) (val - 0.5);

   C8 src[256];
   sprintf(src,"%d",n);

   write_49x_field(buffer, tag, src, suffix);
}

C8 *synthesize_492P(S32 device_address)
{
#define APPEND (&synthesis_buffer[strlen(synthesis_buffer)])
   SA_STATE *SA = SA_startup();

   SA_parse_command_line("-t");  

   if (!SA_connect(device_address))
      {
      SA_shutdown();
      return NULL;
      }

   S32 n_passes = 1;

   GPIB_print("SAVEA?;");
   C8 *text = GPIB_read_ASC();

   if (!strncmp(text,"SAVEA ON",8))
      {
      n_passes = 2;
      }

   SA->hi_speed_acq = FALSE;
   SA_fetch_trace();

   DOUBLE dest_array[2][W_GRAT_49X];
   S32    n_points;

   if (n_passes == 1)
      {
      n_points = SA->n_trace_points;

      SA_resample_data(SA->dBm_values, n_points,
                       dest_array[0],  W_GRAT_49X,
                       RT_POINT);
      }
   else
      {
      n_points = SA->n_trace_points / 2;
      assert(SA->n_trace_points == n_points * 2);

      for (S32 i=0; i < 2; i++)
         {
         DOUBLE src_array[2][W_GRAT_49X];

         for (S32 j=0; j < n_points; j++)
            {
            src_array[i][j] = SA->dBm_values[(j*2)+i];
            }

         SA_resample_data(src_array[i],  n_points,
                          dest_array[i], W_GRAT_49X,
                          RT_POINT);
         }
      }

   synthesis_buffer[0] = 0;

   for (S32 c=0; c < n_passes; c++)
      {
      S32 i = 0;
   
      strcat(synthesis_buffer,c ? "SP5;" : "SP3;");

      bool PD_written = FALSE;
      bool PA_written = FALSE;

      while (1)
         {
         if (i < W_GRAT_49X)
            {
            if (!PD_written)
               {
               sprintf(APPEND, "\nPU;PA %d,%d;PD;",
                  x_to_49x_scale(SA, i),
                  y_to_49x_scale(SA, dest_array[c][i]));
               PD_written = TRUE;
               PA_written = FALSE;
               }
            else
               {
               if (!PA_written)
                  {
                  sprintf(APPEND,"PA ");
                  PA_written = TRUE;
                  }
   
               sprintf(APPEND, "%d,%d,",
                  x_to_49x_scale(SA, i),
                  y_to_49x_scale(SA, dest_array[c][i]));
               }
            }
         else
            {
            S32 l = strlen(trace_buffer);
   
            if (trace_buffer[l-1] == ',')
               {
               trace_buffer[l-1] = ';';
               }
   
            if (i == W_GRAT_49X)
               {
               break;
               }
   
            PA_written = FALSE;
            PD_written = FALSE;
            }
   
         i++;
         }
      }

   strcpy(trace_buffer, (C8 *) TEK49X_plotData);

   write_49x_field(trace_buffer,"*FRQ",freq_text(SA->CF_Hz, FT_LSD_MHZ), "");
   write_49x_field(trace_buffer,"*SPN",freq_text((SA->max_Hz - SA->min_Hz) / 10, FT_LSD_INT), "");
   write_49x_int  (trace_buffer,"*REF",SA->max_dBm, " dBm");
   write_49x_int  (trace_buffer,"*DIV",SA->dB_division, " dB/");
   write_49x_field(trace_buffer,"*RBW",freq_text(SA->RBW_Hz, FT_LSD_INT), "");

   if (SA->VBW_Hz >= 0.0)
      write_49x_field(trace_buffer,"*VBW",freq_text(SA->VBW_Hz, FT_LSD_INT), ""); 
   else
      write_49x_field(trace_buffer,"*VBW",freq_text(SA->RBW_Hz, FT_LSD_INT), "");

   C8 ID_buffer[512];
   strcpy(ID_buffer, &SA->ID_string[7]); 
   ID_buffer[4] = 0;

   write_49x_field(trace_buffer,"*MFG","TEK","");
   write_49x_field(trace_buffer,"*IDN",ID_buffer,"");

   DOUBLE val = SA->max_dBm;

   for (S32 i=0; i < 9; i++)
      {
      C8 tag[3] = "*0";
      tag[1] = (C8) (i + '0');

      write_49x_int(trace_buffer,tag,val,"");

      val -= SA->dB_division;
      }

   strcat(trace_buffer, synthesis_buffer);
   strcat(trace_buffer, "PU;PA0,1000;SP0");

   SA_shutdown();
   return trace_buffer;
#undef APPEND
}

#define SWAP16(y) ((((y) & 0xff00) >> 8) + (((y) & 0x00ff) << 8))
#define XDEV(x)   ((SINGLE(x) * 8.415F) + 1391.0F)
#define YDEV(y)   ((SINGLE(y) * 8.426F) + 844.0F)

C8 *synthesize_856xA(S32  device_address, bool is_278X)
{
#define APPEND (&trace_buffer[strlen(trace_buffer)])
   S32 i,c;

   GPIB_set_EOS_mode(-1, FALSE);
   strcpy(synthesis_buffer, HP8566_plot);

   U8 state[80];
   memset(state, 0, sizeof(state));

   if (!is_278X)
      {
      GPIB_print("OL");      
      S32 actual_len = 0;
      C8 *response = GPIB_read_BIN(80,TRUE,FALSE,&actual_len);
      if (actual_len != 80) return NULL;
      memcpy(state, response, 80);
      }
   else
      {
      C8 *result = GPIB_query("TRDSP TRNOR?");
      _strupr(result);
      if (strstr(result,"OFF") != NULL) state[20] |= 4;

      result = GPIB_query("TRDSP TRA?");
      _strupr(result);
      if (strstr(result,"OFF") != NULL) state[20] |= 32;
      }

   S32 trace_blanked[2] = { state[20] & 4, state[20] & 32 };

   static C8 OT_buffer[(32*64)+1];
   memset(OT_buffer, 0, sizeof(OT_buffer));

   GPIB_print("OT");
   C8 *result = GPIB_read_ASC(sizeof(OT_buffer)-1);
   if (result == NULL) return NULL;
   strcpy(OT_buffer, result);

   C8 *src = OT_buffer;
   for (i=0; i < 31; i++)
      {
      C8 *s = src;
      src = strchr(src, 0x0D);
      if (src == NULL) return NULL;
      *src = 0; src++; src++;

      C8 marker[8];
      sprintf(marker,"xxxx%02d",i+1);
      C8 *tag = strstr(synthesis_buffer, marker);
      if (tag == NULL) continue;

      C8 *next = strchr(tag, 0x0A);
      next++;

      while ((tag[-1] != 'B') || (tag[-2] != 'L')) tag--;

      while (*s)
         {
         if ((s[0] == ' ') && (s[1] == ' ')  && (s[2] == 'H') && (s[3] == 'z'))
            {
            *tag++ = ' '; *tag++ = 'H'; *tag++ = 'z';
            s += 4;
            }
         else if ((s[0] == 'R') && (s[1] == 'E')  && (s[2] == 'F') && (s[3] == ' ') && (s[4] == ' '))
            {
            *tag++ = 'R'; *tag++ = 'E'; *tag++ = 'F'; *tag++ = ' ';
            s += 5;
            }
         else if ((s[0] == ' ') && (s[1] == ' ') && (s[2] == ' '))
            {
            *tag++ = 13; *tag++ = 10;
            while (*s == ' ') s++;
            }
         else if (s[0] == -7)
            {
            strcpy(tag,"DELTA"); tag += 5; s++;
            }
         else if ((s[0] > 0x7e) || (s[0] < ' '))
            {
            *tag++ = isprint(s[0]) ? '?' : ' ';
            s++;
            }
         else
            {
            *tag++ = *s++;
            }
         }

      *tag++ = 3;
      memmove(tag, next, strlen(next)+1);
      }

   static U16 curve[2][1001];
   for (c=0; c < 2; c++)
      {
      if (trace_blanked[c]) continue;
      if (!is_278X) GPIB_print(c ? "O2;TB" : "O2;TA");
      else          GPIB_print(c ? "O2;TROUT TRA" : "O2;TROUT TRNOR");

      memset(curve[c], 0, 1001 * sizeof(U16));
      memcpy(curve[c], GPIB_read_BIN(1001 * sizeof(U16),TRUE,FALSE), 1001 * sizeof(U16));

      if (is_278X) GPIB_read_ASC();  
      }

   static SINGLE trace[2][1001];
   S32 valid[2] = { -1, -1 };

   for (c=0; c < 2; c++)
      {
      if (trace_blanked[c]) continue;
      for (i=0; i < 1001; i++)
         {
         curve[c][i] = SWAP16(curve[c][i]);
         trace[c][i] = YDEV(curve[c][i]);
         if ((curve[c][i] < 1024) && (valid[c] == -1)) valid[c] = i;
         }
      }

   trace_buffer[0] = 0;
   for (c=0; c < 2; c++)
      {
      if (valid[c] == -1) continue;
      strcat(trace_buffer,c ? "SP5;" : "SP3;");
      i = valid[c];
      bool PD_written = FALSE;
      bool PA_written = FALSE;

      while (1)
         {
         if ((i < 1001) && (curve[c][i] < 1024))
            {
            if (!PD_written)
               {
               sprintf(APPEND, "\nPU;PA %.0f,%.0f;PD;",XDEV(i),trace[c][i]);
               PD_written = TRUE; PA_written = FALSE;
               }
            else
               {
               if (!PA_written) { sprintf(APPEND,"PA "); PA_written = TRUE; }
               sprintf(APPEND, "%.0f,%.0f,",XDEV(i),trace[c][i]);
               }
            }
         else
            {
            S32 l = strlen(trace_buffer);
            if (trace_buffer[l-1] == ',') trace_buffer[l-1] = ';';
            if (i == 1001) break;
            PA_written = FALSE; PD_written = FALSE;
            }
         i++;
         }
      }

   return trace_buffer;
#undef APPEND
}

C8 *synthesize_3585A(S32 device_address)
{
#define APPEND (&trace_buffer[strlen(trace_buffer)])
   S32 i,j;
   strcpy(synthesis_buffer, HP8566_plot);
   GPIB_set_EOS_mode(10, FALSE);

   static C8 annotation[10][64];
   memset(annotation, 0, sizeof(annotation));
   static C8 original_annotation[10][64];
   memset(original_annotation, 0, sizeof(original_annotation));

   GPIB_print("D7T4");

   for (i=0; i < 10; i++)
      {
      C8 *result = GPIB_read_ASC(sizeof(annotation[i])-1);
      assert(result != NULL);
      for (j=0; j < (S32) strlen(result); j++)
         {
         if (!isspace((U8) result[j])) { strcpy(original_annotation[i],&result[j]); break; }
         }
      strcpy(annotation[i], result);
      _strupr(annotation[i]);
      for (j=strlen(annotation[i])-1; j >= 0; j--)    
         if (isspace((U8) annotation[i][j])) strcpy(&annotation[i][j], &annotation[i][j+1]);
      }

   S32 marker_visible = FALSE;
   SINGLE SOURCE_min_dBm     = 10000.0F;
   SINGLE SOURCE_max_dBm     = 10000.0F;
   S32    SOURCE_dB_division = 10000;
   DOUBLE SOURCE_min_Hz      = -FLT_MAX;
   DOUBLE SOURCE_max_Hz      = -FLT_MAX;
   DOUBLE MARKER_Hz          = -FLT_MAX;
   SINGLE MARKER_dBm         = 10000.0F;

   sscanf(annotation[2], "%dDB/DIV", &SOURCE_dB_division);

   if (SOURCE_dB_division != 10000)
      {
      sscanf(annotation[0], "REF%fDBM", &SOURCE_max_dBm);
      SOURCE_min_dBm = SOURCE_max_dBm - (SOURCE_dB_division * 10);
      if ((strstr(annotation[5],"START") != NULL) && (strstr(annotation[6],"STOP") != NULL))
         {
         sscanf(annotation[5], "START%lf", &SOURCE_min_Hz);
         sscanf(annotation[6], "STOP%lf", &SOURCE_max_Hz);
              if (strstr(annotation[5], "GHZ")) SOURCE_min_Hz *= 1.0E9;
         else if (strstr(annotation[5], "MHZ")) SOURCE_min_Hz *= 1.0E6;
         else if (strstr(annotation[5], "KHZ")) SOURCE_min_Hz *= 1.0E3;
              if (strstr(annotation[6], "GHZ")) SOURCE_max_Hz *= 1.0E9;
         else if (strstr(annotation[6], "MHZ")) SOURCE_max_Hz *= 1.0E6;
         else if (strstr(annotation[6], "KHZ")) SOURCE_max_Hz *= 1.0E3;
         marker_visible = TRUE;
         }
      else if ((strstr(annotation[5],"CENTER") != NULL) && (strstr(annotation[6],"SPAN") != NULL)) 
         {
         DOUBLE SOURCE_center_Hz = -FLT_MAX;
         DOUBLE SOURCE_span_Hz   = -FLT_MAX;
         sscanf(annotation[5], "CENTER%lf", &SOURCE_center_Hz);
         sscanf(annotation[6], "SPAN%lf", &SOURCE_span_Hz);
              if (strstr(annotation[5], "GHZ")) SOURCE_center_Hz *= 1.0E9;
         else if (strstr(annotation[5], "MHZ")) SOURCE_center_Hz *= 1.0E6;
         else if (strstr(annotation[5], "KHZ")) SOURCE_center_Hz *= 1.0E3;
              if (strstr(annotation[6], "GHZ")) SOURCE_span_Hz *= 1.0E9;
         else if (strstr(annotation[6], "MHZ")) SOURCE_span_Hz *= 1.0E6;
         else if (strstr(annotation[6], "KHZ")) SOURCE_span_Hz *= 1.0E3;
         SOURCE_span_Hz /= 2.0;
         SOURCE_min_Hz = SOURCE_center_Hz - SOURCE_span_Hz;
         SOURCE_max_Hz = SOURCE_center_Hz + SOURCE_span_Hz; 
         marker_visible = TRUE;
         }
      if (marker_visible)
         if (((SOURCE_max_Hz  - SOURCE_min_Hz)  < 0.1) || ((SOURCE_max_dBm - SOURCE_min_dBm) < 0.1))
            marker_visible = FALSE;
      }

   if (marker_visible)
      {
      C8 marker_text[64];
      memset(marker_text, 0, sizeof(marker_text));
      GPIB_print("D2T4");
      C8 *result = GPIB_read_ASC(sizeof(marker_text)-1);
      assert(result != NULL);
      strcpy(marker_text, result); _strupr(marker_text);
      for (j=strlen(marker_text)-1; j >= 0; j--)    
         if (isspace((U8) marker_text[j])) strcpy(&marker_text[j], &marker_text[j+1]);
      sscanf(marker_text, "%lf,%f", &MARKER_Hz, &MARKER_dBm);
      if ((MARKER_Hz == -FLT_MAX) || (MARKER_dBm == 10000.0F)) marker_visible = FALSE;
      else
         {
         if (MARKER_Hz  < SOURCE_min_Hz)  MARKER_Hz  = SOURCE_min_Hz;
         if (MARKER_Hz  > SOURCE_max_Hz)  MARKER_Hz  = SOURCE_max_Hz;
         if (MARKER_dBm < SOURCE_min_dBm) MARKER_dBm = SOURCE_min_dBm;
         if (MARKER_dBm > SOURCE_max_dBm) MARKER_dBm = SOURCE_max_dBm;
         }
      }

   for (S32 OT=1; OT <= 32; OT++)
      {
      C8 marker[8]; sprintf(marker,"xxxx%02d",OT);
      C8 *tag = strstr(synthesis_buffer, marker);
      if (tag == NULL) continue;
      C8 *next = strchr(tag, 0x0A); next++;
      while ((tag[-1] != 'B') || (tag[-2] != 'L')) tag--;
      const S32 source_string[33] = { -1, -1, -1,  7,  8,  9,  3,  0, 2, -1,  5,  6, -1, -1, -1,  1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
      S32 sn = source_string[OT];
      if (sn != -1)
         {
         C8 *s = &original_annotation[sn][0];
         while (*s)
            {
            if ((s[0] == ' ') && (s[1] == ' ') && (s[2] == ' ')) { *tag++ = 13; *tag++ = 10; while (*s == ' ') s++; }
            else if (s[0] == -7) { strcpy(tag,"DELTA"); tag += 5; s++; }
            else if ((s[0] > 0x7e) || (s[0] < ' ')) { *tag++ = isprint(s[0]) ? '?' : ' '; s++; }
            else { *tag++ = *s++; }
            }
         }
      *tag++ = 3; memmove(tag, next, strlen(next)+1);
      }

   GPIB_print("SABO");
   static U16 curve[1002];                               
   memset(curve, 0, 1002 * sizeof(S16));
   memcpy(curve, GPIB_read_BIN(1002 * sizeof(S16),TRUE,FALSE), 1002 * sizeof(S16));
   for (i=0; i < 1001; i++) curve[i+1] = ((curve[i+1] & 0xff00) >> 8) + ((curve[i+1] & 0x0003) << 8);

   GPIB_print("TB0");
   static SINGLE dspcoord[1001];
   for (i=0; i < 1001; i++) dspcoord[i] = YDEV(curve[i+1]);

   trace_buffer[0] = 0; strcat(trace_buffer,"SP3;");
   bool PD_written = FALSE; bool PA_written = FALSE;
   i = 0;
   while (1)
      {
      if ((i < 1001) && (curve[i] < 1024))
         {
         if (!PD_written) { sprintf(APPEND, "\nPU;PA %.0f,%.0f;PD;",XDEV(i),dspcoord[i]); PD_written = TRUE; PA_written = FALSE; }
         else { if (!PA_written) { sprintf(APPEND,"PA "); PA_written = TRUE; } sprintf(APPEND, "%.0f,%.0f,",XDEV(i),dspcoord[i]); }
         }
      else
         {
         S32 l = strlen(trace_buffer);
         if (trace_buffer[l-1] == ',') trace_buffer[l-1] = ';';
         if (i == 1001) break;
         PA_written = FALSE; PD_written = FALSE;
         }
      i++;
      }
   sprintf(APPEND, "PU;\n");

   if (marker_visible)
      {
      DOUBLE dsp_mx = (MARKER_Hz  - SOURCE_min_Hz ) / (SOURCE_max_Hz  - SOURCE_min_Hz);
      SINGLE dsp_my = (MARKER_dBm - SOURCE_min_dBm) / (SOURCE_max_dBm - SOURCE_min_dBm);
      S32 x = (S32) XDEV(dsp_mx * 1000.0);
      S32 y = (S32) YDEV(dsp_my * 1000.0);
      sprintf(APPEND, "SP2;PA %d,%d;PD;PA %d,%d;", x-100, y+130, x+100, y-130);
      sprintf(APPEND, "PU;PA %d,%d;PD;PA %d,%d;", x+100, y+130, x-100, y-130);
      sprintf(APPEND,"PU;\n");
      }
   strcat(trace_buffer, synthesis_buffer);
   C8 HP_logo[]  = "PA250,9307; SR1.549,2.323; UC-99,3,-1,99,2,9,-99,-1,-4,99,3,0,-1,-5,-99;  UC-99,0,-5,99,2,9,3,0,-1,-5,-3,0,-99;";
   sprintf(APPEND,"%s", HP_logo);
   return trace_buffer;
#undef APPEND
}

#define X_GRAT_8566 1391
#define Y_GRAT_8566 844
#define W_GRAT_8566 8415
#define H_GRAT_8566 8426

static S32 x_to_8566_scale(SA_STATE *SA, S32 n)   
{
   S32 x = X_GRAT_8566 + n;
   if (x >= X_GRAT_8566 + W_GRAT_8566) x = X_GRAT_8566 + W_GRAT_8566 - 1;
   if (x <  X_GRAT_8566)          x = X_GRAT_8566;
   return x;
}

static S32 y_to_8566_scale(SA_STATE *SA, DOUBLE dBm) 
{
   S32 y = Y_GRAT_8566 + (S32) (((dBm - SA->min_dBm) * (F32) H_GRAT_8566) / (SA->max_dBm - SA->min_dBm));
   if (y >= Y_GRAT_8566 + H_GRAT_8566) y = Y_GRAT_8566 + H_GRAT_8566 - 1;
   if (y <  Y_GRAT_8566)          y = Y_GRAT_8566;
   return y;
}

C8 *synthesize_generic_SA(S32 device_address, bool SCPI)
{
#define APPEND  (&trace_buffer[strlen(trace_buffer)])
   SA_STATE *SA = SA_startup();
   SA_parse_command_line("-t -G");  
   if (SCPI) SA_parse_command_line("-scpi");
   if (!SA_connect(device_address)) { SA_shutdown(); return NULL; }
   SA_fetch_trace();

   static DOUBLE dest_array[W_GRAT_8566];
   SA_resample_data(SA->dBm_values, SA->n_trace_points, dest_array, W_GRAT_8566, RT_SPLINE); 

   static C8 annotation[10][64];
   memset(annotation, 0, sizeof(annotation));
   sprintf(annotation[0],"Start %s",      freq_text(SA->min_Hz, FT_LSD_HZ));     
   sprintf(annotation[1],"Stop %s",       freq_text(SA->max_Hz, FT_LSD_HZ));      
   sprintf(annotation[2],"Ref %0.1f dBm", SA->max_dBm);                     
   sprintf(annotation[3],"%d dB/",        SA->dB_division);                        
   sprintf(annotation[5],"Center %s",     freq_text(SA->CF_Hz, FT_LSD_HZ));     

   if (SA->RBW_Hz     >= 0.0)     sprintf(annotation[4],"Res BW %s",     freq_text(SA->RBW_Hz, FT_LSD_INT));   
   if (SA->sweep_secs >= 0)       sprintf(annotation[6],"Sweep %0.3f s", SA->sweep_secs);
   if (SA->VBW_Hz     >= 0.0)     sprintf(annotation[7],"Vid BW %s",     freq_text(SA->VBW_Hz, FT_LSD_INT));   
   if (SA->RFATT_dB   > -10000.0) sprintf(annotation[8],"RF Att %d dB",  (S32) (SA->RFATT_dB + 0.5F));                     
   if (SA->vid_avgs   > 0)        sprintf(annotation[9],"Vid Avgs\n %d",  SA->vid_avgs);

   strcpy(synthesis_buffer, HP8566_plot);
   for (S32 OT=1; OT <= 32; OT++)
      {
      C8 marker[8]; sprintf(marker,"xxxx%02d",OT);
      C8 *tag = strstr(synthesis_buffer, marker);
      if (tag == NULL) continue;
      C8 *next = strchr(tag, 0x0A); next++;
      while ((tag[-1] != 'B') || (tag[-2] != 'L')) tag--;
      const S32 source_string[33] = { -1, -1, -1,  4,  7,  6,  8,  2, 3, -1,  0,  1, -1, -1, -1,  5, -1, -1,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
      S32 sn = source_string[OT];               
      if (sn != -1) { C8 *s = &annotation[sn][0]; while (*s) *tag++ = *s++; }
      *tag++ = 3; memmove(tag, next, strlen(next)+1);
      }

   strcpy(trace_buffer,"SP3;");
   bool PD_written = FALSE; bool PA_written = FALSE;
   S32 i = 0;
   while (1)
      {
      if (i < W_GRAT_8566)
         {
         if (!PD_written) { sprintf(APPEND, "\nPU;PA %d,%d;PD;", x_to_8566_scale(SA, i), y_to_8566_scale(SA, dest_array[i])); PD_written = TRUE; PA_written = FALSE; }
         else { if (!PA_written) { sprintf(APPEND,"PA "); PA_written = TRUE; } sprintf(APPEND, "%d,%d,", x_to_8566_scale(SA, i), y_to_8566_scale(SA, dest_array[i])); }
         }
      else
         {
         S32 l = strlen(trace_buffer);
         if (trace_buffer[l-1] == ',') trace_buffer[l-1] = ';';
         if (i == W_GRAT_8566) break;
         PA_written = FALSE; PD_written = FALSE;
         }
      i++;
      }
   strcat(trace_buffer, synthesis_buffer);
   SA_shutdown();
   return trace_buffer;
#undef APPEND
}
