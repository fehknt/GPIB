#pragma once

struct WINCON_static_constructor
{
   WINCON_static_constructor()
      {
      //
      // Redirect stdout to debug console if present
      //
      // WinCon.exe must be running, kernel32.dll must be available,
      // and AttachConsole function must be available... otherwise we
      // don't do anything
      //

      HWND hWC = FindWindow(NULL,"wincon");

      if (hWC != NULL)
         {
         HMODULE module = LoadLibrary("kernel32.dll");

         if (module != NULL)
            {
            typedef BOOL (NTAPI *ATTACHCONSOLEFN)(DWORD);
            ATTACHCONSOLEFN attach_console = (ATTACHCONSOLEFN) GetProcAddress(module, "AttachConsole");

            if (attach_console != NULL)
               {
               DWORD pid = 0;
               GetWindowThreadProcessId(hWC, &pid);
               SetForegroundWindow(hWC);     // optional

               attach_console(pid);
               freopen("CONOUT$","wb",stdout);
               // puts("\x1b[01m\x1b[31m");     // usable if ANSI supported
               puts("\n-------------------\nDebug output begins\n-------------------\n");
               // puts("\x1b[01m\x1b[0m");
               }

            FreeLibrary(module);
            }
         }
      }

   ~WINCON_static_constructor()
      {
      puts("\n-----------------\nDebug output ends\n-----------------\n");
      }
};

static WINCON_static_constructor _WINCON_static_constructor;
