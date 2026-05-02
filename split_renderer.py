
import re
import os

header_path = r'C:\Users\fehkn\OneDrive\Documents\github repos\GPIB\include\renderer.h'
cpp_path = r'C:\Users\fehkn\OneDrive\Documents\github repos\GPIB\src\common\renderer.cpp'

with open(header_path, 'r') as f:
    lines = f.readlines()

new_header = []
new_cpp = ['#include "renderer.h"\n', '#include <ctype.h>\n', '#include <math.h>\n']

# We need to find global variables and functions.
# This is tricky with regex in C++, but we can try.

in_function = False
function_content = []
braces_level = 0

# Variables to move (definitions with initializers or just definitions)
# We will look for lines that are not inside functions and look like variable definitions.

def is_variable_def(line):
    line = line.strip()
    if not line: return False
    if line.startswith('//'): return False
    if line.startswith('#'): return False
    if line.startswith('extern'): return False
    if '(' in line and ')' in line and '{' not in line and ';' not in line: return False # likely a prototype
    if '(' in line and '{' in line: return False # function start
    if '=' in line and ';' in line: return True # assignment definition
    if ';' in line and not '(' in line: return True # simple definition
    return False

# Actually, a better approach:
# 1. Keep headers, macros, types in header.
# 2. Move all function bodies to CPP.
# 3. Move all global variable definitions to CPP, leave extern in Header.

re_func_start = re.compile(r'^(\w[\w\s\*]*\s+\w+)\s*\((.*?)\)\s*\{')
re_var_def = re.compile(r'^(\w[\w\s\*]*\s+\w+)(\[.*?\])?(\s*=.*?)?;')

# Tables like vg_00[] PROGMEM = { ... };
re_table_def = re.compile(r'^(u08\s+\w+\[\]\s+PROGMEM\s*=)')

# For simplicity, I'll manually mark ranges based on the file content I read.
# But since I have to be autonomous, I'll try to parse.

new_header.append("#pragma once\n")
new_header.append("#include <windows.h>\n")
new_header.append("#include <mmsystem.h>\n")
new_header.append("#include <stdio.h>\n")
new_header.append("#include <stdlib.h>\n")
new_header.append("#include <string.h>\n")
new_header.append("#include <malloc.h>\n")
new_header.append("#include <assert.h>\n")
new_header.append("#include <math.h>\n")
new_header.append("#include <float.h>\n")
new_header.append('#include "typedefs.h"\n\n')
new_header.append("#define u08 unsigned char\n\n")

# ... (I'll skip some headers that I already added)

current_pos = 0
while current_pos < len(lines):
    line = lines[current_pos]
    
    # Skip already added headers
    if line.startswith("#pragma once") or line.startswith("#include") or line.strip() == "#define u08 unsigned char":
        current_pos += 1
        continue
    
    # Handle defines and comments - stay in header mostly
    if line.strip().startswith("#define") or line.strip().startswith("//") or line.strip().startswith("/*") or not line.strip():
        new_header.append(line)
        current_pos += 1
        continue

    # Handle struct/typedef - stay in header
    if line.strip().startswith("typedef struct") or line.strip().startswith("struct"):
        brace_count = 0
        while current_pos < len(lines):
            l = lines[current_pos]
            new_header.append(l)
            brace_count += l.count('{')
            brace_count -= l.count('}')
            current_pos += 1
            if brace_count == 0 and ';' in l:
                break
        continue

    # Handle tables like u08 vg_00[] PROGMEM = { ... };
    if "PROGMEM" in line and "=" in line and "{" in line:
        new_header.append("extern " + line.split('=')[0].strip() + ";\n")
        brace_count = 0
        while current_pos < len(lines):
            l = lines[current_pos]
            new_cpp.append(l)
            brace_count += l.count('{')
            brace_count -= l.count('}')
            current_pos += 1
            if brace_count == 0 and ';' in l:
                break
        continue

    # Handle function implementation
    if '(' in line and ')' in line and not line.strip().endswith(';'):
        # Check if it has a body
        temp_pos = current_pos
        has_body = False
        while temp_pos < len(lines) and temp_pos < current_pos + 5:
            if '{' in lines[temp_pos]:
                has_body = True
                break
            if ';' in lines[temp_pos]:
                break
            temp_pos += 1
        
        if has_body:
            # Move to CPP, leave prototype in Header
            proto = ""
            while current_pos < len(lines):
                l = lines[current_pos]
                if '{' in l:
                    proto += l.split('{')[0]
                    new_header.append(proto.strip() + ";\n")
                    
                    # Move body
                    brace_count = 0
                    while current_pos < len(lines):
                        l = lines[current_pos]
                        new_cpp.append(l)
                        brace_count += l.count('{')
                        brace_count -= l.count('}')
                        current_pos += 1
                        if brace_count == 0:
                            break
                    break
                else:
                    proto += l
                    new_cpp.append(l)
                    current_pos += 1
            continue

    # Handle global variables
    if ';' in line and not '(' in line and not '}' in line:
        # Move to CPP, leave extern in Header
        if "PROGMEM" in line:
            new_header.append("extern " + line.split('=')[0].replace('PROGMEM','').strip() + ";\n")
        else:
            decl = line.split('=')[0].strip()
            # If it has multiple variables like double gl_x, gl_y;
            if ',' in decl:
                parts = decl.split()
                type_part = parts[0]
                vars_part = " ".join(parts[1:])
                new_header.append("extern " + type_part + " " + vars_part + ";\n")
            else:
                new_header.append("extern " + decl + ";\n")
        new_cpp.append(line)
        current_pos += 1
        continue

    # Default: stay in header
    new_header.append(line)
    current_pos += 1

with open(header_path, 'w') as f:
    f.writelines(new_header)

with open(cpp_path, 'w') as f:
    f.writelines(new_cpp)
