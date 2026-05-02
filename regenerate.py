import os
import re
import subprocess

inline_libraries = ['timeutil', 'ipconn', 'comport', 'di154', 'appfile', 'gfxfile', '7470lex', 'renderer', 'spline', 'sparams']
paired_libraries = ['xy', 'recorder', 'waterfall']
other_headers = ['wincon.h', 'dsplib.h', 'gpiblib.h', 'visatype.h', 'visa.h', '49xplt.h', '8566plt.h', 'chartype.h', 'pnres.h', 'prores.h', 'ssmres.h', 'tchkres.h', 'vnares.h', 'w32sal.h', 'winvfx.h']

def get_git_file(filename):
    res = subprocess.run(['git', 'show', f'HEAD:{filename}'], capture_output=True)
    if res.returncode == 0:
        return res.stdout.decode('cp1252', errors='ignore').replace('\x1a', '')
    return ""

def fix_cpp_includes(content):
    return re.sub(r'#include\s+[\"<]([a-zA-Z0-9_-]+)\.cpp[\">]', r'#include "\1.h"', content)

os.makedirs('include', exist_ok=True)
os.makedirs('src/common', exist_ok=True)

safe_includes = "#pragma once\n#include <windows.h>\n#include <mmsystem.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <malloc.h>\n#include <assert.h>\n#include <math.h>\n#include <float.h>\n#include \"typedefs.h\"\n"

# 1. Process inline libraries
for base in inline_libraries:
    content = get_git_file(f"{base}.cpp")
    if not content: continue
    
    dummy_funcs = re.findall(r'void\s+_{10,}[A-Z0-9_]+_{10,}\s*\([^)]*\)\s*\{(?:[^{}]*)\}', content)
    for func in dummy_funcs:
        content = content.replace(func, '')
        
    content = fix_cpp_includes(content)
    
    inc_content = safe_includes + content
    
    with open(f"include/{base}.h", 'w', encoding='utf-8') as f:
        f.write(inc_content)
        
    shim_content = f'#include "{base}.h"\n\n'
    for func in dummy_funcs:
        shim_content += func + '\n\n'
        
    with open(f"src/common/{base}.cpp", 'w', encoding='utf-8') as f:
        f.write(shim_content)

# 2. Process Paired Libraries
for base in paired_libraries:
    h_content = get_git_file(f"{base}.h")
    if h_content:
        h_content = fix_cpp_includes(h_content)
        h_content = safe_includes + h_content
        with open(f"include/{base}.h", 'w', encoding='utf-8') as f:
            f.write(h_content)
            
    c_content = get_git_file(f"{base}.cpp")
    if c_content:
        c_content = fix_cpp_includes(c_content)
        with open(f"src/common/{base}.cpp", 'w', encoding='utf-8') as f:
            f.write(c_content)

# 3. Process other remaining headers
for h in other_headers:
    h_content = get_git_file(h)
    if h_content:
        h_content = fix_cpp_includes(h_content)
        h_content = safe_includes + h_content
        with open(f"include/{h}", 'w', encoding='utf-8') as f:
            f.write(h_content)

# Process typedefs.h naturally
typedefs_content = get_git_file("typedefs.h")
if typedefs_content:
    typedefs_content = fix_cpp_includes(typedefs_content)
    with open(f"include/typedefs.h", 'w', encoding='utf-8') as f:
        f.write(typedefs_content)

print("Regeneration fully automated!")
