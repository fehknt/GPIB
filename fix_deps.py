import os

files = ['appfile', 'gfxfile', '7470lex', 'renderer', 'spline', 'sparams']

includes = '''
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <ctype.h>
#include <assert.h>
#include \"typedefs.h\"
'''

for f in files:
    inc_path = os.path.join('include', f + '.h')
    if not os.path.exists(inc_path):
        continue
    
    with open(inc_path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
    
    # Just insert it after #pragma once
    if '#pragma once' in content:
        content = content.replace('#pragma once', '#pragma once\\n' + includes, 1)
    else:
        content = includes + '\\n' + content
        
    with open(inc_path, 'w', encoding='utf-8') as file:
        file.write(content)

print('Added missing system includes!')
