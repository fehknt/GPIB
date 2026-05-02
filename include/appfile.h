#ifndef APPFILE_H
#define APPFILE_H

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
#include "typedefs.h"
#include "stdtpl.h"
#include "gpib_wincon.h"

// *****************************************************************************
//
// Misc. Windows file utilities
//
// *****************************************************************************

enum TFMSGLVL
{
   TF_DEBUG = 0,  // Debugging traffic
   TF_VERBOSE,    // Low-level notice
   TF_NOTICE,     // Standard notice
   TF_WARNING,    // Warning
   TF_ERROR       // Error
};

struct TEMPFN
{
   C8       path_buffer        [MAX_PATH-32];   // leave some room for 8.3 tempname and user-supplied suffix if any
   C8       original_temp_name [MAX_PATH-32];   // e.g., c:\temp\TF43AE.tmp
   C8       name               [MAX_PATH];      // e.g., c:\temp\TF43AE.tmp.wav (if .wav was passed as suffix)
   bool     keep;
   bool     show;
   bool     active;
   TFMSGLVL verbosity;

   virtual void message_sink(TFMSGLVL level, C8 *text);
   virtual void message_printf(TFMSGLVL level, C8 *fmt, ...);

   TEMPFN(C8 *suffix = NULL, bool keep_files=FALSE, TFMSGLVL v=TF_NOTICE);
   virtual ~TEMPFN();
   virtual bool status(void);
};

struct TEMPFILE : public TEMPFN
{
   FILE *file;

   TEMPFILE(C8      *suffix         = NULL, 
            C8      *file_operation = NULL, 
            bool     keep_files     = FALSE,
            TFMSGLVL verbosity      = TF_NOTICE);

   virtual ~TEMPFILE();
   virtual void close(void);
};

//
// Encapsulate various Windows file/directory behavior and policies
//

struct APPDIRS
{
   C8 EXE         [MAX_PATH];    // Directory containing .exe corresponding to current process       
   C8 DOCS        [MAX_PATH];    // Directory where user documents are to be loaded/saved by default 
   C8 ICW         [MAX_PATH];    // Current working directory when object was created
   C8 VLDATA      [MAX_PATH];    // Directory for user-specific vendor .INI files and data
   C8 VCDATA      [MAX_PATH];    // Directory for vendor-specific .INI files and data common to all users
   C8 VCDOCS      [MAX_PATH];    // Directory for vendor-specific preinstalled data files common to all users
   C8 LOCDATA     [MAX_PATH];    // Directory for user-specific application data (not good for .INI files unless they're genuinely user-specific)
   C8 COMDATA     [MAX_PATH];    // Directory for application-specific .INI files common to all users
   C8 COMDOCS     [MAX_PATH];    // Directory for application-specific preinstalled data files common to all users
   C8 DESKTOP     [MAX_PATH];    // User's desktop

   C8 vendor_name [MAX_PATH];    // Vendor and app name passed to init()
   C8 app_name    [MAX_PATH];

   static void trailslash(C8 *target);

   APPDIRS();

   void init(const C8   *vname  = NULL, 
             const C8   *aname     = NULL, 
             bool        use_VCDOCS   = TRUE);
};

// Generic .INI-style key/value database
#ifdef STDTPL_H

enum KVMSGLVL
{
   KVAL_DEBUG = 0,      // Debugging traffic
   KVAL_VERBOSE,        // Low-level notice
   KVAL_NOTICE,         // Standard notice
   KVAL_WARNING,        // Warning (does not imply failure of operation)
   KVAL_ERROR           // Error (implies failure of operation)
};

#ifndef VALSTR_LEN      // String values are 1024 bytes long by default
#define VALSTR_LEN 1023
#endif                  

const U32 KVAL_MULTIPLE  = 0x00000001;  // Database can contain more than one entry with this key
const U32 KVAL_TEMP      = 0x00000002;  // Key should not be written to any files
const U32 KVAL_READ_ONLY = 0x00000004;  // TRUE if the value can't be updated after creation
const U32 KVAL_REQUIRED  = 0x00000008;  // TRUE if a value must be specified in the list passed to from_args()
const U32 KVAL_BLANK_OK  = 0x00000010;  // Empty string may be provided

enum KVAL_TYPE
{
   KVT_NONE,            // Uninitialized
   KVT_STR,             // String
   KVT_NUM,             // Signed 32-bit int                                   
   KVT_HEX,             // Unsigned 32-bit int represented in string form as a hex value
   KVT_DBL,             // Double-precision float                              
   KVT_NUM64,           // Signed 64-bit int                                   
   KVT_BOOL,            // Boolean
#ifdef GDIPVER
   KVT_COLOR            // Instance of GDI+ Color object
#endif
};

const U32 KVF_FILE_MUST_EXIST = 0x00000001;  // Report error if file does not exist
const U32 KVF_DO_NOT_ADD      = 0x00000002;  // Do not create new entries while reading file, just update existing ones

class KVAL : public HASHLIST_CONTENTS        // Base class for contents stored in a KVSTORE
{
protected:
   C8 *value_string;                         // Cached string representation, not directly accessible to the outside world
   void release_owned_value(void);

public:
   U32         KV_flags;           // Database flags
   KVAL_TYPE   type;               // Stored data type
   U32         user_flags;         // App-specific flags (serialized in keydefs)
   void       *user_ptr;           // App-specific metadata
   void       *valaddr;            // Optional address of backing variable
   const void *defaddr;            // Optional address of default value in application space (not yet initialized by all types, TODO)

   void initialize(const void *V);
   void shutdown(void);

   static bool to_bool(const C8 *string);
   static DOUBLE to_double(const C8 *string);

   C8 *to_keydef(C8 *dest, S32 dest_bytes, C8 *prepend_tag = NULL);
   void to_var(void *val);
   bool from_var(const void *varaddr);
   bool user_var_to_string(void);
   bool set_default(void);
   bool name_match(C8 *match_name);
   const C8 *str(void);
   bool setstr(const C8 *val);
   bool is_blank(void);
   bool sprintf(const C8 *fmt, ...);
   bool vsprintf(const C8 *fmt, va_list ap);
   bool scatf(const C8 *fmt, ...);
   bool sgl(SINGLE *result);
   SINGLE sgl(void);
   bool dbl(DOUBLE *result);
   DOUBLE dbl(void);
   bool setdbl(DOUBLE val);
   bool num(S32 *result);
   S32 num(void);
   bool setnum(S32 val);
   bool num64(S64 *result);
   S64 num64(void);
   bool setnum64(S64 val);
   bool hex(U32 *result);
   U32 hex(void);
   bool sethex(U32 val);
#ifdef GDIPVER
   bool color(Color *result);
   Color color(void);
   bool setcolor(Color val);
#endif
   bool boolean(bool *result);
   bool boolean(void);
   bool setbool(bool val);
};

// Macros used to declare global or local variables backed by KVSTORE entries
#define INI_BOOL(database,varname,defval)       bool   varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addbool (#varname,        &varname); }} varname##_INI_container_;
#define INI_S32(database,varname,defval)        S32    varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addnum  (#varname,        &varname); }} varname##_INI_container_;
#define INI_S64(database,varname,defval)        S64    varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addnum64(#varname,        &varname); }} varname##_INI_container_;
#define INI_DOUBLE(database,varname,defval)     DOUBLE varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.adddbl  (#varname,        &varname); }} varname##_INI_container_;
#define INI_U32(database,varname,defval)        U32    varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addhex  (#varname,        &varname); }} varname##_INI_container_;
#define INI_HEX(database,varname,defval)        U32    varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addhex  (#varname,        &varname); }} varname##_INI_container_;
#define INI_STRING(database,varname,defval)     C8     varname[VALSTR_LEN+1] = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addstr  (#varname,         varname); }} varname##_INI_container_;
#define INI_ENUM(database,type,varname,defval)  type   varname               = defval; struct varname##_INI_init_ { varname##_INI_init_() { database.addhex  (#varname,(U32 *) &varname); }} varname##_INI_container_;

#define ARG_S32(database,varname,defval,KV_flags)    S32    ARG_##varname               = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.addnum  (#varname,        &ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;
#define ARG_S64(database,varname,defval,KV_flags)    S64    ARG_##varname               = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.addnum64(#varname,        &ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;
#define ARG_DOUBLE(database,varname,defval,KV_flags) DOUBLE ARG_##varname               = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.adddbl  (#varname,        &ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;
#define ARG_HEX(database,varname,defval,KV_flags)    U32    ARG_##varname               = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.addhex  (#varname,        &ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;
#define ARG_STRING(database,varname,defval,KV_flags) C8     ARG_##varname[VALSTR_LEN+1] = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.addstr  (#varname,         ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;
#define ARG_BOOL(database,varname,defval,KV_flags)   bool   ARG_##varname               = defval; struct ARG_##varname##_init_ { ARG_##varname##_init_() { database.addbool (#varname,        &ARG_##varname, NULL, 0, KV_flags); }} ARG_##varname##_container_;

template<class KVCONTENTS> class KVSTORE
{
   void init(void)
      {
      contents.clear();
      clear_error();
      }

public:   
   HashList<KVCONTENTS> contents;      
   C8                   error[1024];

   KVSTORE(void)
      {
      }

   virtual ~KVSTORE()
      {
      }

   KVCONTENTS *operator [] (const C8 *key)
      {
      return contents[key];
      }

   KVCONTENTS &operator [] (S32 index)
      {
      return contents[index];
      }

   bool keydef(C8 *text, U32 *flags, C8 **key, C8 **def)
      {
      C8 *q1 = strchr(text,'\'');
      C8 *q2 = strchr(text,'\"');

      if ((q1 == NULL) && (q2 == NULL))
         {
         if (key   != NULL) *key = text;
         if (def   != NULL) *def = &text[strlen(text)];
         if (flags != NULL) *flags = 0;
         return TRUE;
         }

      C8 *k = NULL;

      if ((q1 != NULL) && (q2 != NULL))
         {
         k = (q1 < q2) ? q1 : q2;
         }
      else
         {
         k = (q1 == NULL) ? q2 : q1;

         if (k == NULL)
            {
            message_printf(KVAL_ERROR,"KVAL error: malformed keydef (%s)", text);
            return FALSE;
            }
         }

      C8 *quote = k++;
      C8 *d = strchr(k, *quote);

      if (d == NULL)
         {
         message_printf(KVAL_ERROR,"KVAL error: malformed keydef (%s)", text);
         return FALSE;
         }

      *d++ = 0;
      while (isspace((U8) *d)) d++;

      if (key != NULL) *key = k;
      if (def != NULL) *def = d;

      if (flags != NULL)
         {
         C8 *f = strstr(text, "0x");
         if ((f != NULL) && (f < quote))
            sscanf(f,"0x%X",flags);
         else
            *flags = 0;
         }

      return TRUE;
      }

   void clear_error(void)
      {
      error[0] = 0;
      }

   virtual void message_sink(KVMSGLVL level, C8 *text)
      {
      ::printf("%s\n",text);
      }

   virtual void message_printf(KVMSGLVL level, C8 *fmt, ...)
      {
      C8 buffer[16384] = { 0 };
      va_list ap;
      va_start(ap, fmt);
      _vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
      va_end(ap);

      C8 *end = &buffer[strlen(buffer)-1];
      while (end > buffer)
         {
         if (!isspace((U8) *end)) break;
         *end = 0; end--;
         }

      if (level == KVAL_ERROR)
         {
         memset(error, 0, sizeof(error));
         strncpy(error, buffer, sizeof(error)-1);
         }

      message_sink(level, buffer);
      }

   void delete_entry(const C8 *key)
      {
      KVCONTENTS *V = contents[key];
      if (V != NULL) contents.unlink(V);
      }

   void delete_entry(KVCONTENTS *V)
      {
      contents.unlink(V);
      }

   void delete_all_by_key(C8 *key)
      {
      for (;;)
         {
         KVCONTENTS *V = contents[key];
         if (V == NULL) break;
         contents.unlink(V);
         }
      }

   void clear(void)
      {
      contents.clear();
      }

   S32 count(void)
      {
      return contents.count();
      }

   S32 count_by_key(C8 *key)
      {
      S32 n=0;
      for (S32 i=0; i < contents.count(); i++)
         if (contents[i].name_match(key)) ++n;
      return n;
      }

   void sort(S32 polarity=1)
      {
      contents.sort_simple_contents(polarity);
      }

   KVCONTENTS *lookup(C8 *key)
      {
      return contents[key];
      }

   bool has(C8 *key)
      {
      return (contents[key] != NULL);
      }

   const C8 *str(const C8 *key)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return NULL;
      return V->str();
      }

   bool dbl(const C8 *key, DOUBLE *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->dbl(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: dbl(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   DOUBLE dbl(const C8 *key, DOUBLE fallback = 0.0, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: dbl(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      DOUBLE value = fallback;
      bool result = V->dbl(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: dbl(%s) incompatible type", key);
      return value;
      }

   bool sgl(const C8 *key, SINGLE *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->sgl(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: sgl(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   SINGLE sgl(const C8 *key, SINGLE fallback = 0.0, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: sgl(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      SINGLE value = fallback;
      bool result = V->sgl(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: sgl(%s) incompatible type", key);
      return value;
      }

   bool num(const C8 *key, S32 *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->num(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: num(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   S32 num(const C8 *key, S32 fallback = 0, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: num(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      S32 value = fallback;
      bool result = V->num(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: num(%s) incompatible type", key);
      return value;
      }

   bool hex(const C8 *key, U32 *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->hex(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: hex(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   U32 hex(const C8 *key, U32 fallback = 0, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: hex(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      U32 value = fallback;
      bool result = V->hex(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: hex(%s) incompatible type", key);
      return value;
      }

   bool num64(const C8 *key, S64 *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->num64(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: num64(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   S64 num64(const C8 *key, S64 fallback = 0, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: num64(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      S64 value = fallback;
      bool result = V->num64(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: num64(%s) incompatible type", key);
      return value;
      }

   bool boolean(const C8 *key, bool *result)
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL) return FALSE;
      if (!V->boolean(result))
         {
         message_printf(KVAL_ERROR,"KVAL error: bool(%s) incompatible type", key);
         return FALSE;
         }
      return TRUE;
      }

   bool boolean(const C8 *key, bool fallback = FALSE, bool *outcome = NULL)    
      {
      KVCONTENTS *V = contents[key];
      if (V == NULL)
         {
         if (outcome == NULL) message_printf(KVAL_WARNING, "KVAL warning: bool(%s) not found", key);
         if (outcome) *outcome = FALSE;
         return fallback;
         }
      bool value = fallback;
      bool result = V->boolean(&value);
      if (outcome != NULL) *outcome = result;
      else if (!result) message_printf(KVAL_ERROR,"KVAL error: bool(%s) incompatible type", key);
      return value;
      }

   KVCONTENTS *addstr(const C8 *key, C8 *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL)
            {
            message_printf(KVAL_ERROR,"KVAL error: addstr(%s) already present", key);
            return NULL;
            }
         }
      V = contents.allocate(key);
      if (V == NULL)
         {
         message_printf(KVAL_ERROR,"KVAL error: addstr(%s) out of memory", key);
         return NULL;
         }
      V->type = KVT_STR; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = (initializer != NULL) ? V->setstr(initializer) : V->setstr(valaddr);
      if (!result)
         {
         message_printf(KVAL_ERROR,"KVAL error: addstr(%s) out of memory", key);
         return NULL;
         }
      return V;
      }

   KVCONTENTS *addstr(const C8 *key_def, U32 KV_flags=0)    
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addstr(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

   KVCONTENTS *adddbl(const C8 *key, DOUBLE val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) out of memory", key); return NULL; }
      V->type = KVT_DBL; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->setdbl(val)) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *adddbl(const C8 *key, DOUBLE *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) out of memory", key); return NULL; }
      V->type = KVT_DBL; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setstr(initializer) : V->setdbl(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: adddbl(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *adddbl(const C8 *key_def, U32 KV_flags=0)
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = adddbl(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

   KVCONTENTS *addnum(const C8 *key, S32 val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) out of memory", key); return NULL; }
      V->type = KVT_NUM; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->setnum(val)) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addnum(const C8 *key, S32 *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) out of memory", key); return NULL; }
      V->type = KVT_NUM; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setstr(initializer) : V->setnum(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: addnum(%s) out of memory", key); return NULL; }
      return V;
      }
   
   KVCONTENTS *addnum(const C8 *key_def, U32 KV_flags=0) 
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addnum(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

   KVCONTENTS *addhex(const C8 *key, U32 val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) out of memory", key); return NULL; }
      V->type = KVT_HEX; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->sethex(val)) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addhex(const C8 *key, U32 *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) out of memory", key); return NULL; }
      V->type = KVT_HEX; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setstr(initializer) : V->sethex(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: addhex(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addhex(const C8 *key_def, U32 KV_flags=0)  
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addhex(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

   KVCONTENTS *addnum64(const C8 *key, S64 val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) out of memory", key); return NULL; }
      V->type = KVT_NUM64; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->setnum64(val)) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addnum64(const C8 *key, S64 *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) out of memory", key); return NULL; }
      V->type = KVT_NUM64; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setstr(initializer) : V->setnum64(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: addnum64(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addnum64(const C8 *key_def, U32 KV_flags=0)    
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addnum64(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

   KVCONTENTS *addbool(const C8 *key, bool val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) out of memory", key); return NULL; }
      V->type = KVT_BOOL; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->setbool(val)) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addbool(const C8 *key, bool *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) out of memory", key); return NULL; }
      V->type = KVT_BOOL; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setbool(V->to_bool(initializer)) : V->setbool(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: addbool(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addbool(const C8 *key_def, U32 KV_flags=0)   
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addbool(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }

#ifdef GDIPVER
   KVCONTENTS *addcolor(const C8 *key, Color val, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      V->type = KVT_COLOR; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = NULL;
      if (!V->setcolor(val)) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addcolor(const C8 *key, Color *valaddr, const C8 *initializer=NULL, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      V->type = KVT_COLOR; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr;
      bool result = ((initializer != NULL) || (valaddr == NULL)) ? V->setstr(initializer) : V->setcolor(*valaddr);
      if (!result) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addcolor(const C8 *key, Color *valaddr, const Color *default_initializer, U32 user_flags=0, U32 KV_flags=0)
      {
      KVCONTENTS *V = NULL;
      if (!(KV_flags & KVAL_MULTIPLE))
         {
         V = contents[key];
         if (V != NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) already present", key); return NULL; }
         }
      V = contents.allocate(key);
      if (V == NULL) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      V->type = KVT_COLOR; V->KV_flags = KV_flags; V->user_flags = user_flags; V->valaddr = valaddr; V->defaddr = default_initializer;
      if (!V->setcolor(*default_initializer)) { message_printf(KVAL_ERROR,"KVAL error: addcolor(%s) out of memory", key); return NULL; }
      return V;
      }

   KVCONTENTS *addcolor(const C8 *key_def, U32 KV_flags=0)  
      {
      C8 *text = _strdup(key_def);
      if (text == NULL) return NULL;
      C8 *key=NULL, *def=NULL; U32 flags = 0;
      if (!keydef(text, &flags, &key, &def)) { free(text); return NULL; }
      KVCONTENTS *result = addcolor(key, NULL, def, flags, KV_flags);
      free(text); return result;
      }
#endif

   bool update_value(const C8 *key, const C8 *string, bool *changed = NULL)
      {
      KVCONTENTS *V = contents[key];
      if ((V == NULL) || (V->KV_flags & KVAL_MULTIPLE))
         {
         if (changed != NULL) *changed = TRUE;
         return (addstr(key, NULL, string, 0, KVAL_MULTIPLE) != NULL);  
         }
      return update_value(V, string, changed);
      }

   bool update_value(KVCONTENTS *V, const C8 *string, bool *changed = NULL)
      {
      if (V->KV_flags & KVAL_READ_ONLY)
         {
         message_printf(KVAL_ERROR,"KVAL error: can't update read-only value %s", V->name);
         return FALSE;
         }
      if (changed != NULL) if (strcmp(string, V->str())) *changed = TRUE;
      if (!V->setstr(string))    
         {
         message_printf(KVAL_ERROR,"KVAL error: update() out of memory, key %s", V->name);
         return FALSE;
         }
      return TRUE;
      }

   void vars_to_strings(void)
      {
      for (S32 i=0; i < contents.count(); i++) contents[i].user_var_to_string();
      }

   bool read(const C8 *filename, U32 KVF_flags = 0)
      {
      if ((strchr(filename,':') != NULL) || (strstr(filename,"\\\\") != NULL)) return read("", filename, KVF_flags);
      else return read(".\\", filename, KVF_flags);
      }

   bool read(const C8 *directory_name, const C8 *filename, U32 KVF_flags = 0)
      {
      C8 pathname[MAX_PATH] = "";
      memset(pathname, 0, sizeof(pathname));
      _snprintf(pathname, sizeof(pathname)-1, "%s%s", directory_name, filename);
      FILE *infile = fopen(pathname,"rt");
      if (infile == NULL) { if (KVF_flags & KVF_FILE_MUST_EXIST) message_printf(KVAL_ERROR,"File '%s' not found", pathname); return FALSE; }
      bool result = read(infile, KVF_flags);
      fclose(infile); return result;
      }

   bool read(FILE *infile, U32 KVF_flags = 0)
      {
      for (;;)
         {
         C8 linbuf[2048] = { 0 };
         C8 *result = fgets(linbuf, sizeof(linbuf) - 1, infile);
         if (result == NULL) break;
         S32 l = strlen(linbuf);
         if ((!l) || (linbuf[0] == ';')) continue;
         C8 *key = linbuf; C8 *end = linbuf; bool leading = TRUE;
         for (S32 i=0; i < l; i++) { if (!isspace((U8) linbuf[i])) { if (leading) { key = &linbuf[i]; leading = FALSE; } end = &linbuf[i]; } }
         end[1] = 0; if (leading || (!strlen(key))) continue;
         C8 EOK_char = ' '; if ((key[0] == '\'') || (key[0] == '\"')) { EOK_char = key[0]; key++; }
         C8 *value = strchr(key, EOK_char);
         if (value == NULL) value = "";
         else { *value++ = 0; while (isspace((U8) (*value))) value++; }
         if (KVF_flags & KVF_DO_NOT_ADD) { KVCONTENTS *V = contents[key]; if (V == NULL) continue; }
         if (!update_value(key, value)) { fclose(infile); return FALSE; }
         }
      return TRUE;
      }

   bool write(const C8 *filename)
      {
      if ((strchr(filename,':') != NULL) || (strstr(filename,"\\\\") != NULL)) return write("", filename);
      else return write(".\\", filename);
      }

   bool write(const C8 *directory_name, const C8 *filename, bool keep_existing_entries=FALSE)
      {
      C8 destname[MAX_PATH] = "";
      memset(destname, 0, sizeof(destname));
      _snprintf(destname, sizeof(destname)-1, "%s%s", directory_name, filename);
      FILE *outfile = fopen(destname,"wt");
      if (outfile == NULL) { message_printf(KVAL_ERROR,"File '%s' could not be opened for writing", destname); return FALSE; }
      if (keep_existing_entries)
         {
         KVSTORE existing; existing.read(destname);
         for (S32 i=0; i < existing.contents.count(); i++) { KVCONTENTS *E = &existing.contents[i]; if (contents[E->name] == NULL) if (!write(outfile, E)) { fclose(outfile); return FALSE; } }
         }
      for (S32 i=0; i < contents.count(); i++) { KVCONTENTS *V = &contents[i]; if (!(V->KV_flags & KVAL_TEMP)) if (!write(outfile, V)) { fclose(outfile); return FALSE; } }
      fclose(outfile); return TRUE;
      }

   bool write(FILE *outfile, KVCONTENTS *V)
      {
      if (V->KV_flags & KVAL_TEMP) return FALSE;
      if (strchr(V->name, '\"') == NULL) fprintf(outfile,"\"%s\" %s\n", V->name, V->str());
      else fprintf(outfile,"'%s' %s\n", V->name, V->str());
      if (ferror(outfile)) { message_printf(KVAL_ERROR,"Write failure -- disk full?"); return FALSE; }
      return TRUE;
      }

   void show_usage(const C8 *usage, bool leading_CRLF);
   bool from_args(S32 argc, const C8 * const *argv, const C8 *usage, S32 max_anon_values=0, S32 required_anon_values=0);

   bool copy_to(KVSTORE *D, bool include_addresses=FALSE)
      {
      for (S32 i=0; i < contents.count(); i++)
         {
         KVAL *SV = &contents[i];
         KVAL *DV = D->lookup(SV->name); 
         if (DV == NULL) { DV = D->contents.allocate(SV->name); if (DV == NULL) { message_printf(KVAL_ERROR,"Out of memory"); return FALSE; } DV->type = SV->type; DV->user_flags = SV->user_flags; DV->user_ptr = SV->user_ptr; DV->KV_flags = SV->KV_flags; if (include_addresses) DV->valaddr = SV->valaddr; }
         if (!DV->setstr(SV->str())) { message_printf(KVAL_ERROR,"Out of memory"); return FALSE; }
         }
      return TRUE;
      }

   KVSTORE & operator= (KVSTORE &src)
      {
      if (&src == this) return *this;
      clear(); src.copy_to(this);
      return *this;
      }

   KVSTORE(KVSTORE &src)
      {
      clear(); src.copy_to(this);
      }
};

template<class KVCONTENTS> void KVSTORE<KVCONTENTS>::show_usage(const C8 *usage, bool leading_CRLF)
{
   C8 work[8192] = { 0 };
   const C8 *s = usage;
   C8 *d = work;
   C8 cur_key[256] = { 0 };
   bool has_default_placeholders = (strchr(usage,'$') != NULL);
   while ((*s) && (d < &work[sizeof(work)-1]))
   {
      if (*s == '\n') cur_key[0] = 0;
      if (has_default_placeholders && ((*s == '/') || (*s == '-')) && (cur_key[0] == 0))    
      {
         C8 *t = cur_key; const C8 *c = s; c++;
         while ((*c) && (!isspace((U8) *c)) && (*c != ':') && (*c != '=') && (t < &cur_key[sizeof(cur_key)-1])) *t++ = *c++;
         *t++ = 0;
      }
      if ((*s == '$') && (cur_key[0] != 0))
      {
         s++; KVCONTENTS *V = contents[cur_key]; cur_key[0] = 0;
         if (V != NULL) { const C8 *c = V->str(); while ((*c) && (d < &work[sizeof(work)-1])) *d++ = *c++; }
      }
      else *d++ = *s++;
   }
   *d++ = 0;
   if (leading_CRLF) message_printf(KVAL_NOTICE, "\n%s", work);
   else message_printf(KVAL_NOTICE, "%s", work);
}

template<class KVCONTENTS> bool KVSTORE<KVCONTENTS>::from_args(S32 argc, const C8 * const *argv, const C8 *usage, S32 max_anon_values, S32 required_anon_values)
{
   if ((argc > 1) && ((argv[1][0] == '-') || (argv[1][0] == '/')) && ((argv[1][1] == '?') )) { show_usage(usage, FALSE); return FALSE; }
   S32 anon_key = 1; C8 key[MAX_PATH] = { 0 }; C8 val[MAX_PATH] = { 0 };
   for (S32 i=1; i < argc; i++)
   {
      memset(key, 0, sizeof(key)); memset(val, 0, sizeof(val));
      const C8 *A = argv[i];
      if ((A[0] != '-') && (A[0] != '/'))
      {
         if (anon_key > max_anon_values) { show_usage(usage, TRUE); message_printf(KVAL_ERROR,"\nUnrecognized argument (\"%s\")", A); return FALSE; }
         sprintf(key,"%d",anon_key++); strncpy(val, A, sizeof(val)-1);
         if (addstr(key, NULL, val, 0, 0) == NULL) return FALSE;
      }
      else
      {
         const C8 *v = strchr(A, ':'); if (v == NULL) v = strchr(A, '=');
         if (v == NULL) strncpy(key, &A[1], sizeof(key)-1);
         else { strncpy(key, &A[1], min(max(0,(int)(v-A-1)), (int)(sizeof(key)-1))); strncpy(val, v+1, sizeof(val)-1); }
      }
      KVCONTENTS *V = contents[key];
      if (V == NULL) { show_usage(usage, TRUE); message_printf(KVAL_ERROR,"\nUnrecognized argument (\"%s\")", key); return FALSE; }
      if (V->user_flags) { show_usage(usage, TRUE); message_printf(KVAL_ERROR,"\nDuplicate arguments (\"%s\")", key); return FALSE; }
      if ((!val[0]) && (V->type != KVT_BOOL) && (!(V->KV_flags & KVAL_BLANK_OK))) { show_usage(usage, TRUE); message_printf(KVAL_ERROR,"\nValue must be supplied for argument \"%s\"", key); return FALSE; }
      if (!update_value(V, val)) return FALSE;
      V->user_flags = TRUE;
   }
   C8 missing_args[8192] = { 0 }; C8 *d = missing_args; S32 n_missing = 0;
   for (S32 i=0; i < contents.count(); i++)
   {
      KVCONTENTS *V = &contents[i];
      if ((V->KV_flags & KVAL_REQUIRED) && (!V->user_flags))
      {
         if (missing_args[0]) d += _snprintf(d, sizeof(missing_args)-(d-missing_args+1), ", ");
         d += _snprintf(d, sizeof(missing_args)-(d-missing_args+1), "\"%s\"", V->name); ++n_missing;
      }
   }
   if (n_missing) { show_usage(usage, TRUE); if (n_missing > 1) message_printf(KVAL_ERROR,"\nMissing arguments (%s)", missing_args); else message_printf(KVAL_ERROR,"\nMissing argument (%s)", missing_args); return FALSE; }
   if (required_anon_values >= anon_key) { show_usage(usage, TRUE); message_printf(KVAL_ERROR,"\nMissing argument"); return FALSE; }
   return TRUE;
}

#ifdef Button_GetCheck

enum KVDC
{  
   KVDC_TEXT,
   KVDC_DOUBLE,
   KVDC_S32,
   KVDC_RADIOBTN,
   KVDC_CHECKBOX,
   KVDC_LISTBOX
};

struct KVDLGFIELD : public KVAL
{
   KVDC ctrl_type;
   S32  ctrl_ID;
   C8   ctrl_name [512];
   C8   ctrl_init [4096];
   C8   min_str[512];
   C8   max_str[512];
   C8   def_str[512];
   bool ctrl_enabled;

   void initialize(const void *V);
   C8 *clamp(C8 *test_value = NULL);
};

struct KVDLGSTORE : public KVSTORE<KVDLGFIELD>
{
   void to_dialog(HWND hDlg, bool set_enable_states=TRUE);
   bool from_dialog(HWND hDlg, bool *changed = NULL);
};

#define INI_DLG_STRING(ctrlname,ctrlID,varname,userflags,defval) \
   struct varname##_container_ \
      { \
      C8 value[VALSTR_LEN+1]; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->addstr(ctrlname, value, defval, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_TEXT; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         } \
      varname##_container_() { D = NULL; F = NULL; value[0] = 0; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      C8 * operator = (const C8 *v) { F->setstr(v); return value; } \
      } \
   varname;

#define INI_DLG_DOUBLE(ctrlname,ctrlID,varname,userflags,defval,minval,maxval) \
   struct varname##_container_ \
      { \
      DOUBLE value; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->adddbl(ctrlname, &value, defval, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_DOUBLE; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         strcpy(F->max_str, maxval); \
         strcpy(F->min_str, minval); \
         strcpy(F->def_str, defval); \
         } \
      varname##_container_() { D = NULL; F = NULL; value = 0.0; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      DOUBLE operator = (DOUBLE v) { F->setdbl(v); return v; } \
      } \
   varname;

#define INI_DLG_S32(ctrlname,ctrlID,varname,userflags,defval,minval,maxval) \
   struct varname##_container_ \
      { \
      S32 value; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->addnum(ctrlname, &value, NULL, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_S32; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         strcpy(F->max_str, maxval); \
         strcpy(F->min_str, minval); \
         sprintf(F->def_str, "%d", defval); \
         } \
      varname##_container_() { D = NULL; F = NULL; value = defval; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      S32 operator = (S32 v) { F->setnum(v); return v; } \
      } \
   varname;

#define INI_DLG_RADIOBTN(ctrlname,ctrlID,varname,userflags,defval) \
   struct varname##_container_ \
      { \
      bool value; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->addbool(ctrlname, &value, NULL, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_RADIOBTN; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         } \
      varname##_container_() { D = NULL; F = NULL; value = defval; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      bool operator = (bool v) { F->setbool(v); return v; } \
      } \
   varname;

#define INI_DLG_CHECKBOX(ctrlname,ctrlID,varname,userflags,defval) \
   struct varname##_container_ \
      { \
      bool value; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->addbool(ctrlname, &value, NULL, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_CHECKBOX; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         } \
      varname##_container_() { D = NULL; F = NULL; value = defval; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      bool operator = (bool v) { F->setbool(v); return v; } \
      } \
   varname;

#define INI_DLG_LISTBOX(ctrlname,ctrlID,varname,userflags,defval,initstr) \
   struct varname##_container_ \
      { \
      S32 value; \
      KVDLGFIELD *F; \
      KVDLGSTORE *D; \
      void store(KVDLGSTORE *database) \
         { \
         D = database; \
         F = D->addnum(ctrlname, &value, NULL, userflags); \
         if (F == NULL) return; \
         F->ctrl_type = KVDC_LISTBOX; \
         F->ctrl_ID = ctrlID; \
         strcpy(F->ctrl_name, ctrlname); \
         strcpy(F->ctrl_init, initstr); \
         } \
      varname##_container_() { D = NULL; F = NULL; value = defval; } \
      ~varname##_container_() { if (F != NULL) { D->delete_entry(F); F = NULL; } } \
      S32 operator = (S32 v) { F->setnum(v); return v; } \
      } \
   varname;

#endif // Button_GetCheck

#ifdef GDIPVER
#define INI_RGB(database,varname,desc_text,def_R,def_G,def_B) \
   Color varname (def_R, def_G, def_B); \
   const Color varname##default(def_R, def_G, def_B); \
   struct varname##_INI_init_ \
   { \
      varname##_INI_init_() \
         { \
         KVAL *K = database.addcolor(#varname, &varname, &varname##default); \
         if (K != NULL) K->user_ptr = desc_text;   \
         } \
   } varname##_INI_container;
#endif // GDIPVER

#endif // STDTPL_H

#endif // APPFILE_H
