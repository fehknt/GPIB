import os
import re
import glob
import shutil

# Files known to act as header-only common libraries that need ODR-splitting
inline_libraries = ['appfile', 'gfxfile', '7470lex', 'renderer', 'spline', 'sparams']

# Files that already have .h and .cpp
paired_libraries = ['xy', 'recorder', 'waterfall']

# 1. Move all .h files to include/
for h in glob.glob('*.h'):
    shutil.move(h, os.path.join('include', h))

# 2. Process paired libraries (their .h was just moved)
for p in paired_libraries:
    cpp_file = p + '.cpp'
    if os.path.exists(cpp_file):
        shutil.move(cpp_file, os.path.join('src/common', cpp_file))

# 3. Process inline libraries
for base in inline_libraries:
    old_cpp = base + '.cpp'
    if not os.path.exists(old_cpp):
        continue
        
    with open(old_cpp, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Move content to inc_path
    inc_path = os.path.join('include', base + '.h')
    
    if '#pragma once' not in content:
        content = '#pragma once\n' + content
        
    dummy_funcs = re.findall(r'void\s+_{10,}[A-Z0-9_]+_{10,}\s*\([^)]*\)\s*\{(?:[^{}]*)\}', content)
    for func in dummy_funcs:
        content = content.replace(func, '')
        
    with open(inc_path, 'w', encoding='utf-8') as f:
        f.write(content)
        
    # Write shim to src/common
    shim_path = os.path.join('src/common', old_cpp)
    shim_content = f'#include "{base}.h"\n\n'
    for func in dummy_funcs:
        shim_content += func + '\n\n'
        
    with open(shim_path, 'w', encoding='utf-8') as f:
        f.write(shim_content)
        
    os.remove(old_cpp)

# 4. Move all remaining .cpp files to src/apps as standalone applications
# But wait, what if someone is a DLL library?
remaining_cpps = glob.glob('*.cpp')
for app in remaining_cpps:
    shutil.move(app, os.path.join('src/apps', app))

# 5. Fix up #include ".*\.cpp" across the whole src/ and include/ tree
def rewrite_includes(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Replace #include "foo.cpp" with #include "foo.h"
    # Match both quotes and angle brackets just in case
    new_content = re.sub(r'#include\s+[\"<]([a-zA-Z0-9_-]+)\.cpp[\">]', r'#include "\1.h"', content)
    
    if new_content != content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
            print(f"Updated includes in {filepath}")

for d in ['src', 'include']:
    for root, _, files in os.walk(d):
        for f in files:
            if f.endswith('.cpp') or f.endswith('.h'):
                rewrite_includes(os.path.join(root, f))

print("Phase 2 migration successfully executed!")
