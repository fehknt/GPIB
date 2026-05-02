#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <algorithm>
#include "typedefs.h"
#include "gpib_wincon.h"
#include "appfile.h"

// TEMPFN methods
void TEMPFN::message_sink(TFMSGLVL level, C8 *text)
{
   ::printf("%s\n", text);
}

void TEMPFN::message_printf(TFMSGLVL level, C8 *fmt, ...)
{
   if (level < verbosity)
   {
      return;
   }

   C8 buffer[4096] = { 0 };
   va_list ap;
   va_start(ap, fmt);
   _vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);
   va_end(ap);

   C8 *end = &buffer[strlen(buffer) - 1];
   while (end > buffer)
   {
      if (!isspace((U8)*end))
      {
         break;
      }
      *end = 0;
      end--;
   }
   message_sink(level, buffer);
}

TEMPFN::TEMPFN(C8 *suffix, bool keep_files, TFMSGLVL v)
{
   keep = keep_files;
   active = TRUE;
   path_buffer[0] = 0;
   original_temp_name[0] = 0;
   name[0] = 0;
   verbosity = v;

   if (!GetTempPath(sizeof(path_buffer) - 1, path_buffer))
   {
      message_printf(TF_ERROR, "TEMPFN: GetTempPath() failed, code 0x%X", GetLastError());
      active = FALSE;
      return;
   }

   if (!GetTempFileName(path_buffer, "TF", 0, original_temp_name))
   {
      message_printf(TF_ERROR, "TEMPFN: GetTempFileName() failed, code 0x%X", GetLastError());
      active = FALSE;
      return;
   }

   strcpy(name, original_temp_name);

   if (suffix == NULL)
   {
      original_temp_name[0] = 0;
   }
   else
   {
      strncat(name, suffix, 15);
   }

   message_printf(TF_VERBOSE, "TEMPFN: original_temp_name: %s", original_temp_name);
   message_printf(TF_VERBOSE, "TEMPFN: name: %s", name);
}

TEMPFN::~TEMPFN()
{
   if (!keep)
   {
      if (original_temp_name[0])
      {
         message_printf(TF_VERBOSE, "TEMPFN: Deleting %s", original_temp_name);
         _unlink(original_temp_name);
         original_temp_name[0] = 0;
      }

      if (name[0])
      {
         message_printf(TF_VERBOSE, "TEMPFN: Deleting %s", name);
         _unlink(name);
         name[0] = 0;
      }
   }
}

bool TEMPFN::status(void)
{
   return active;
}

// TEMPFILE methods
TEMPFILE::TEMPFILE(C8 *suffix, C8 *file_operation, bool keep_files, TFMSGLVL verbosity)
   : TEMPFN(suffix, keep_files, verbosity)
{
   if (file_operation == NULL)
   {
      file = NULL;
      return;
   }

   file = fopen(name, file_operation);

   if (file == NULL)
   {
      message_printf(TF_ERROR, "TEMPFILE: Couldn't open %s for %s",
         name,
         (tolower((U8)file_operation[0]) == 'r') ? "reading" : "writing");

      active = FALSE;
      return;
   }
}

TEMPFILE::~TEMPFILE()
{
   close();
}

void TEMPFILE::close(void)
{
   if (file != NULL)
   {
      fclose(file);
      file = NULL;
   }
}

// APPDIRS methods
void APPDIRS::trailslash(C8 *target)
{
   if (target[strlen(target) - 1] != '\\')
   {
      strcat(target, "\\");
   }
}

APPDIRS::APPDIRS()
{
   EXE[0] = 0;
   DOCS[0] = 0;
   ICW[0] = 0;
   VLDATA[0] = 0;
   VCDATA[0] = 0;
   VCDOCS[0] = 0;
   LOCDATA[0] = 0;
   COMDATA[0] = 0;
   DESKTOP[0] = 0;
}

void APPDIRS::init(const C8 *vname, const C8 *aname, bool use_VCDOCS)
{
   strcpy(vendor_name, vname);
   strcpy(app_name, aname);

   if (GetCurrentDirectory(sizeof(ICW), ICW))
      trailslash(ICW);

   if (!GetModuleFileName(NULL, EXE, sizeof(EXE) - 1))
      strcpy(EXE, ".\\");
   else
   {
      C8 *path = strrchr(EXE, '\\');
      if (path != NULL)
         path[1] = 0;
      else
         strcpy(EXE, ".\\");
   }

   C8 local_appdata[MAX_PATH] = "";
   C8 common_appdata[MAX_PATH] = "";
   C8 common_docs[MAX_PATH] = "";
   C8 mydocs[MAX_PATH] = "";

   SHGetSpecialFolderPath(HWND_DESKTOP, DESKTOP, CSIDL_DESKTOPDIRECTORY, FALSE);
   SHGetSpecialFolderPath(HWND_DESKTOP, mydocs, CSIDL_PERSONAL, FALSE);
   SHGetSpecialFolderPath(HWND_DESKTOP, local_appdata, CSIDL_LOCAL_APPDATA, FALSE);
   SHGetSpecialFolderPath(HWND_DESKTOP, common_appdata, CSIDL_COMMON_APPDATA, FALSE);
   SHGetSpecialFolderPath(HWND_DESKTOP, common_docs, CSIDL_COMMON_DOCUMENTS, FALSE);

   trailslash(DESKTOP);
   trailslash(mydocs);
   trailslash(local_appdata);
   trailslash(common_appdata);
   trailslash(common_docs);

   strcpy(VCDATA, common_appdata);
   if ((vendor_name != NULL) && (vendor_name[0] != 0))
   {
      strcat(VCDATA, vendor_name);
      trailslash(VCDATA);
      CreateDirectory(VCDATA, NULL);
   }

   strcpy(COMDATA, VCDATA);
   if ((app_name != NULL) && (app_name[0] != 0))
   {
      strcat(COMDATA, app_name);
      trailslash(COMDATA);
      CreateDirectory(COMDATA, NULL);
   }

   strcpy(VCDOCS, common_docs);
   if ((vendor_name != NULL) && (vendor_name[0] != 0))
   {
      strcat(VCDOCS, vendor_name);
      trailslash(VCDOCS);
      CreateDirectory(VCDOCS, NULL);
   }

   if (use_VCDOCS)
      strcpy(COMDOCS, VCDOCS);
   else
      strcpy(COMDOCS, common_docs);

   if ((app_name != NULL) && (app_name[0] != 0))
   {
      strcat(COMDOCS, app_name);
      trailslash(COMDOCS);
      CreateDirectory(COMDOCS, NULL);
   }

   strcpy(VLDATA, local_appdata);
   if ((vendor_name != NULL) && (vendor_name[0] != 0))
   {
      strcat(VLDATA, vendor_name);
      trailslash(VLDATA);
      CreateDirectory(VLDATA, NULL);
   }

   strcpy(LOCDATA, VLDATA);
   if ((app_name != NULL) && (app_name[0] != 0))
   {
      strcat(LOCDATA, app_name);
      trailslash(LOCDATA);
      CreateDirectory(LOCDATA, NULL);
   }

   strcpy(DOCS, ICW);
   if ((!strlen(DOCS)) || (!_stricmp(DOCS, DESKTOP)) || (strstr(DOCS, "Program Files") != NULL))
   {
      strcpy(DOCS, mydocs);
      SetCurrentDirectory(DOCS);
   }

   printf("Desktop:  %s\n", DESKTOP);
   printf("VLDATA:   %s\n", VLDATA);
   printf("VCDATA:   %s\n", VCDATA);
   printf("VCDOCS:   %s\n", VCDOCS);
   printf("LOCDATA:  %s\n", LOCDATA);
   printf("COMDATA:  %s\n", COMDATA);
   printf("COMDOCS:  %s\n", COMDOCS);
   printf("AppExe:   %s\n", EXE);
   printf("InitCWD:  %s\n", ICW);
   printf("Docs/CWD: %s\n", DOCS);
}

// String helper functions
static C8 *next_space(C8 *src)
{
   while (*src && (!isspace((U8)*src)))
   {
      ++src;
   }
   return src;
}

static C8 *skip_space(C8 *src)
{
   while (isspace((U8)*src))
   {
      ++src;
   }
   return src;
}

static C8 *next_substring(C8 *src)
{
   src = next_space(src);
   if (!*src)
   {
      return src;
   }
   *src++ = 0;
   return skip_space(src);
}

// KVAL methods
void KVAL::release_owned_value(void)
{
   if (value_string != NULL)
   {
      free(value_string);
      value_string = NULL;
   }
}

void KVAL::initialize(const void *V)
{
   HASHLIST_CONTENTS::initialize(V);
   value_string = NULL;
   valaddr = NULL;
   defaddr = NULL;
   user_flags = 0;
   user_ptr = NULL;
   KV_flags = 0;
   type = KVT_NONE;
}

void KVAL::shutdown(void)
{
   HASHLIST_CONTENTS::shutdown();
   release_owned_value();
}

bool KVAL::to_bool(const C8 *string)
{
   U8 *src = (U8 *)string;
   for (;;)
   {
      U8 v = (U8)tolower(*src++);
      if (isspace(v))
      {
         continue;
      }
      if (v == 0)
      {
         break;
      }
      return (v != '0') && (v != 'f');
   }
   return TRUE;
}

DOUBLE KVAL::to_double(const C8 *string)
{
   S32 l = (S32)strlen(string);
   C8 *buff = (C8 *)alloca(l + 1);
   strcpy(buff, string);

   for (S32 i = 0; i < l; i++)
   {
      if (buff[i] == ' ')
      {
         U8 ch = 0;
         S32 j = i + 1;
         while (j < l)
         {
            ch = (U8)buff[j];
            if (ch != ' ') break;
            j++;
         }
         if ((j == l) || (ch == 0))
         {
            break;
         }
         C8 *nxt = &buff[j];
         memmove(&buff[i], nxt, strlen(nxt) + 1);
      }
   }

   DOUBLE dbl_val = 0.0;
   const C8 *divisor = strchr(buff, '/');
   if (divisor != NULL)
   {
      DOUBLE a = 0.0, b = 1.0;
      sscanf(buff, "%lf", &a);
      sscanf(&divisor[1], "%lf", &b);
      if (fabs(b) < 1E-30)
      {
         return 0.0;
      }
      return a / b;
   }

   const C8 *mult = strchr(buff, '*');
   if (mult != NULL)
   {
      DOUBLE a = 0.0, b = 0.0;
      sscanf(buff, "%lf", &a);
      sscanf(&mult[1], "%lf", &b);
      return a * b;
   }

   sscanf(buff, "%lf", &dbl_val);
   return dbl_val;
}

C8 *KVAL::to_keydef(C8 *dest, S32 dest_bytes, C8 *prepend_tag)
{
   memset(dest, 0, dest_bytes);
   if (prepend_tag == NULL)
   {
      _snprintf(dest, dest_bytes - 1, "0x%08X \"%s\" %s", user_flags, name, str());
   }
   else
   {
      if (prepend_tag[0] != 0)
      {
         _snprintf(dest, dest_bytes - 1, "%s 0x%08X \"%s\" %s", prepend_tag, user_flags, name, value_string);
      }
      else
      {
         switch (type)
         {
         case KVT_STR:   _snprintf(dest, dest_bytes - 1, "STR 0x%08X \"%s\" %s", user_flags, name, value_string); break;
         case KVT_NUM:   _snprintf(dest, dest_bytes - 1, "S32 0x%08X \"%s\" %s", user_flags, name, value_string); break;
         case KVT_HEX:   _snprintf(dest, dest_bytes - 1, "HEX 0x%08X \"%s\" %s", user_flags, name, value_string); break;
         case KVT_DBL:   _snprintf(dest, dest_bytes - 1, "DBL 0x%08X \"%s\" %s", user_flags, name, value_string); break;
         case KVT_NUM64: _snprintf(dest, dest_bytes - 1, "S64 0x%08X \"%s\" %s", user_flags, name, value_string); break;
         case KVT_BOOL:  _snprintf(dest, dest_bytes - 1, "BLN 0x%08X \"%s\" %s", user_flags, name, value_string); break;
#ifdef GDIPVER
         case KVT_COLOR: _snprintf(dest, dest_bytes - 1, "RGB 0x%08X \"%s\" %s", user_flags, name, value_string); break;
#endif
         default: break;
         }
      }
   }
   return dest;
}

void KVAL::to_var(void *val)
{
   if (val == NULL)
   {
      return;
   }

   switch (type)
   {
   case KVT_STR:
   {
      strcpy((C8 *)val, value_string);
      break;
   }
   case KVT_NUM:
   {
      *(S32 *)val = 0;
      sscanf(value_string, "%d", (S32 *)val);
      break;
   }
   case KVT_HEX:
   {
      *(U32 *)val = 0;
      sscanf(value_string, "0x%X", (U32 *)val);
      break;
   }
#ifdef GDIPVER
   case KVT_COLOR:
   {
      S32 A = 255, R = 255, G = 255, B = 0;
      if (toupper(value_string[0]) == 'A')
         sscanf(value_string, "ARGB(%d,%d,%d,%d)", &A, &R, &G, &B);
      else
         sscanf(value_string, "RGB(%d,%d,%d)", &R, &G, &B);
      *(Color *)val = Color(255, (BYTE)R, (BYTE)G, (BYTE)B);
      break;
   }
#endif
   case KVT_DBL:
   {
      *(DOUBLE *)val = to_double(value_string);
      break;
   }
   case KVT_NUM64:
   {
      *(S64 *)val = 0;
      sscanf(value_string, "%I64d", (S64 *)val);
      break;
   }
   case KVT_BOOL:
   {
      *(bool *)val = to_bool(value_string);
      break;
   }
   default:
   {
      break;
   }
   }
}

bool KVAL::from_var(const void *varaddr)
{
   if (varaddr == NULL)
   {
      return FALSE;
   }
   assert(varaddr != value_string);
   bool result = FALSE;
   switch (type)
   {
   case KVT_STR:   result = setstr((const C8 *)varaddr); break;
   case KVT_NUM:   result = setnum(*(const S32 *)varaddr); break;
   case KVT_HEX:   result = sethex(*(const U32 *)varaddr); break;
   case KVT_DBL:   result = setdbl(*(const DOUBLE *)varaddr); break;
   case KVT_NUM64: result = setnum64(*(const S64 *)varaddr); break;
   case KVT_BOOL:  result = setbool(*(const bool *)varaddr); break;
#ifdef GDIPVER
   case KVT_COLOR: result = setcolor(*(const Color *)varaddr); break;
#endif
   default: break;
   }
   return result;
}

bool KVAL::user_var_to_string(void)
{
   return from_var(valaddr);
}

bool KVAL::set_default(void)
{
   return from_var(defaddr);
}

bool KVAL::name_match(C8 *match_name)
{
   return !lex_compare(match_name);
}

const C8 *KVAL::str(void)
{
   return value_string;
}

bool KVAL::setstr(const C8 *val)
{
   release_owned_value();
   value_string = (val == NULL) ? _strdup("") : _strdup(val);
   if (value_string == NULL)
   {
      return FALSE;
   }
   to_var(valaddr);
   return TRUE;
}

bool KVAL::is_blank(void)
{
   return (value_string == NULL) || (value_string[0] == 0);
}

bool KVAL::sprintf(const C8 *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   bool result = vsprintf(fmt, ap);
   va_end(ap);
   return result;
}

bool KVAL::vsprintf(const C8 *fmt, va_list ap)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, VALSTR_LEN + 1);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _vsnprintf(value_string, VALSTR_LEN, fmt, ap);
   to_var(valaddr);
   return TRUE;
}

bool KVAL::scatf(const C8 *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   bool result = FALSE;
   if (is_blank())
   {
      release_owned_value();
      value_string = (C8 *)calloc(1, VALSTR_LEN + 1);
      if (value_string != NULL)
      {
         _vsnprintf(value_string, VALSTR_LEN, fmt, ap);
         to_var(valaddr);
         result = TRUE;
      }
   }
   else
   {
      C8 *old_string = _strdup(value_string);
      if (old_string != NULL)
      {
         release_owned_value();
         value_string = (C8 *)calloc(1, VALSTR_LEN + 1);
         if (value_string != NULL)
         {
            strcpy(value_string, old_string);
            S32 len = (S32)strlen(value_string);
            _vsnprintf(&value_string[len], std::max<int>(0, VALSTR_LEN - len), fmt, ap);
            to_var(valaddr);
            result = TRUE;
         }
         free(old_string);
      }
   }
   va_end(ap);
   return result;
}

bool KVAL::sgl(SINGLE *result)
{
   if (type != KVT_DBL) return FALSE;
   DOUBLE v = 0.0;
   to_var(&v);
   *result = (SINGLE)v;
   return TRUE;
}

SINGLE KVAL::sgl(void)
{
   SINGLE result = 0.0;
   sgl(&result);
   return result;
}

bool KVAL::dbl(DOUBLE *result)
{
   if (type != KVT_DBL) return FALSE;
   to_var(result);
   return TRUE;
}

DOUBLE KVAL::dbl(void)
{
   DOUBLE result = 0.0;
   dbl(&result);
   return result;
}

bool KVAL::setdbl(DOUBLE val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 32);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 31, "%.16lG", val);
   to_var(valaddr);
   return TRUE;
}

bool KVAL::num(S32 *result)
{
   if (type != KVT_NUM) return FALSE;
   to_var(result);
   return TRUE;
}

S32 KVAL::num(void)
{
   S32 result = 0;
   num(&result);
   return result;
}

bool KVAL::setnum(S32 val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 16);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 15, "%d", val);
   to_var(valaddr);
   return TRUE;
}

bool KVAL::num64(S64 *result)
{
   if (type != KVT_NUM64) return FALSE;
   to_var(result);
   return TRUE;
}

S64 KVAL::num64(void)
{
   S64 result = 0;
   num64(&result);
   return result;
}

bool KVAL::setnum64(S64 val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 32);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 31, "%I64d", val);
   to_var(valaddr);
   return TRUE;
}

bool KVAL::hex(U32 *result)
{
   if (type != KVT_HEX) return FALSE;
   to_var(result);
   return TRUE;
}

U32 KVAL::hex(void)
{
   U32 result = 0;
   hex(&result);
   return result;
}

bool KVAL::sethex(U32 val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 16);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 15, "0x%X", val);
   to_var(valaddr);
   return TRUE;
}

#ifdef GDIPVER
bool KVAL::color(Color *result)
{
   if (type != KVT_COLOR) return FALSE;
   to_var(result);
   return TRUE;
}

Color KVAL::color(void)
{
   Color result(255, 255, 255, 0);
   color(&result);
   return result;
}

bool KVAL::setcolor(Color val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 32);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 31, "RGB(%d,%d,%d)", val.GetR(), val.GetG(), val.GetB());
   to_var(valaddr);
   return TRUE;
}
#endif

bool KVAL::boolean(bool *result)
{
   if (type != KVT_BOOL) return FALSE;
   to_var(result);
   return TRUE;
}

bool KVAL::boolean(void)
{
   bool result = FALSE;
   boolean(&result);
   return result;
}

bool KVAL::setbool(bool val)
{
   release_owned_value();
   value_string = (C8 *)calloc(1, 8);
   if (value_string == NULL)
   {
      return FALSE;
   }
   _snprintf(value_string, 7, "%s", val ? "True" : "False");
   to_var(valaddr);
   return TRUE;
}

#ifdef Button_GetCheck
void KVDLGFIELD::initialize(const void *V)
{
   KVAL::initialize(V);
   ctrl_type = KVDC_TEXT;
   ctrl_ID = 0;
   ctrl_name[0] = 0;
   ctrl_init[0] = 0;
   min_str[0] = 0;
   max_str[0] = 0;
   def_str[0] = 0;
   ctrl_enabled = TRUE;
}

C8 *KVDLGFIELD::clamp(C8 *test_value)
{
   if ((ctrl_type != KVDC_DOUBLE) && (ctrl_type != KVDC_S32))
   {
      return NULL;
   }
   if (test_value == NULL)
   {
      test_value = value_string;
   }
   if ((test_value == NULL) || (!test_value[0]))
   {
      return def_str;
   }
   DOUBLE dbl_val = KVAL::to_double(test_value);
   C8 *mx = max_str;
   C8 *mn = min_str;
   bool max_abs = (mx[0] == '|'); if (max_abs) { ++mx; dbl_val = fabs(dbl_val); }
   bool min_abs = (mn[0] == '|'); if (min_abs) { ++mn; dbl_val = fabs(dbl_val); }
   DOUBLE max_val = KVAL::to_double(mx);
   DOUBLE min_val = KVAL::to_double(mn);
   if (ctrl_type == KVDC_S32)
   {
      max_val += 0.01;
      min_val -= 0.01;
   }
   if (dbl_val < min_val)
      return min_str;
   else if (dbl_val > max_val)
      return max_str;
   return NULL;
}

void KVDLGSTORE::to_dialog(HWND hDlg, bool set_enable_states)
{
   for (S32 i = 0; i < contents.count(); i++)
   {
      KVDLGFIELD *E = &contents[i];
      if (E->ctrl_ID == 0)
      {
         continue;
      }
      HWND hDT = GetDlgItem(hDlg, E->ctrl_ID);
      if (hDT == NULL)
      {
         continue;
      }
      switch (E->ctrl_type)
      {
      case KVDC_TEXT:
      case KVDC_DOUBLE:
      case KVDC_S32:
      {
         C8 *clamped_value = E->clamp();
         if (clamped_value != NULL)
         {
            update_value(E->name, clamped_value);
         }
         assert(SetDlgItemText(hDlg, E->ctrl_ID, E->str()));
         if (set_enable_states) EnableWindow(GetDlgItem(hDlg, E->ctrl_ID), E->ctrl_enabled);
         break;
      }
      case KVDC_RADIOBTN:
      case KVDC_CHECKBOX:
      {
         if (E->boolean())
            Button_SetCheck(hDT, TRUE);
         else
            Button_SetCheck(hDT, FALSE);
         if (set_enable_states) EnableWindow(GetDlgItem(hDlg, E->ctrl_ID), E->ctrl_enabled);
         break;
      }
      case KVDC_LISTBOX:
      {
         SendMessage(hDT, CB_RESETCONTENT, 0, 0);
         C8 *src = E->ctrl_init;
         while (*src)
         {
            C8 buffer[512];
            C8 *dest = buffer;
            while ((*src != '\n') && (*src)) *dest++ = *src++;
            while (*src == '\n')                       src++;
            *dest++ = 0;
            SendMessage(hDT, CB_ADDSTRING, 0, (LPARAM)buffer);
         }
         SendMessage(hDT, CB_SETCURSEL, E->num(), 0);
         if (set_enable_states) EnableWindow(GetDlgItem(hDlg, E->ctrl_ID), E->ctrl_enabled);
         break;
      }
      }
   }
}

bool KVDLGSTORE::from_dialog(HWND hDlg, bool *changed)
{
   for (S32 i = 0; i < contents.count(); i++)
   {
      C8 buffer[512] = { 0 };
      KVDLGFIELD *E = &contents[i];
      if (E->ctrl_ID == 0)
      {
         continue;
      }
      HWND hDT = GetDlgItem(hDlg, E->ctrl_ID);
      if ((hDT == NULL) || (!E->ctrl_enabled))
      {
         continue;
      }
      switch (E->ctrl_type)
      {
      case KVDC_TEXT:
      case KVDC_DOUBLE:
      case KVDC_S32:
      {
         GetDlgItemText(hDlg, E->ctrl_ID, buffer, sizeof(buffer) - 1);
         if ((buffer[0] == 0) && (E->ctrl_type != KVDC_TEXT))
         {
            if ((!IsWindowEnabled(hDT)) || (E->min_str[0] == 0) || (E->max_str[0] == 0))
            {
               continue;
            }
            if (E->def_str[0])
               message_printf(KVAL_ERROR, "%s invalid: enter a value from %s to %s (default = %s)", E->ctrl_name, E->min_str, E->max_str, E->def_str);
            else
               message_printf(KVAL_ERROR, "%s invalid: enter a value from %s to %s", E->ctrl_name, E->min_str, E->max_str);
            SetFocus(hDT);
            return FALSE;
         }
         if (E->ctrl_type == KVDC_S32)
         {
            for (S32 j = 0; j < (S32)strlen(buffer); j++)
            {
               if ((!isdigit((U8)buffer[j])) && (buffer[j] != '-') && (buffer[j] != '+'))
               {
                  if (E->def_str[0])
                     message_printf(KVAL_ERROR, "%s must be an integer value from %s to %s (default = %s)", E->ctrl_name, E->min_str, E->max_str, E->def_str);
                  else
                     message_printf(KVAL_ERROR, "%s must be an integer value from %s to %s", E->ctrl_name, E->min_str, E->max_str);
                  SetFocus(hDT);
                  return FALSE;
               }
            }
            DOUBLE val = KVAL::to_double(buffer);
            if (val < 0.0) val -= 0.5;
            if (val > 0.0) val += 0.5;
            sprintf(buffer, "%d", (S32)val);
         }
         if (E->clamp(buffer) != NULL)
         {
            if (E->def_str[0])
               message_printf(KVAL_ERROR, "%s out of range (%s): enter a value from %s to %s (default = %s)", E->ctrl_name, buffer, E->min_str, E->max_str, E->def_str);
            else
               message_printf(KVAL_ERROR, "%s out of range (%s): enter a value from %s to %s", E->ctrl_name, buffer, E->min_str, E->max_str);
            SetFocus(hDT);
            return FALSE;
         }
         break;
      }
      }
   }
   for (S32 i = 0; i < contents.count(); i++)
   {
      C8 buffer[512];
      memset(buffer, 0, sizeof(buffer));
      KVDLGFIELD *E = &contents[i];
      if (E->ctrl_ID == 0)
      {
         continue;
      }
      HWND hDT = GetDlgItem(hDlg, E->ctrl_ID);
      if ((hDT == NULL) || (!E->ctrl_enabled))
      {
         continue;
      }
      switch (E->ctrl_type)
      {
      case KVDC_TEXT:
      case KVDC_DOUBLE:
      case KVDC_S32:
      {
         GetDlgItemText(hDlg, E->ctrl_ID, buffer, sizeof(buffer) - 1);
         if (E->ctrl_type == KVDC_S32)
         {
            DOUBLE val = KVAL::to_double(buffer);
            if (val < 0.0) val -= 0.5;
            if (val > 0.0) val += 0.5;
            sprintf(buffer, "%d", (S32)val);
         }
         if (!update_value(E->name, buffer, changed))
         {
            return FALSE;
         }
         break;
      }
      case KVDC_RADIOBTN:
      case KVDC_CHECKBOX:
      {
         C8 *v = Button_GetCheck(hDT) ? "True" : "False";
         if (!update_value(E->name, v, changed))
         {
            return FALSE;
         }
         break;
      }
      case KVDC_LISTBOX:
      {
         LRESULT sel = SendMessage(hDT, CB_GETCURSEL, 0, 0);
         if (changed != NULL)
         {
            if (E->num() != (S32)sel)
            {
               *changed = TRUE;
            }
         }
         if (!E->setnum((S32)sel))
         {
            return FALSE;
         }
         break;
      }
      }
   }
   return TRUE;
}
#endif // Button_GetCheck
