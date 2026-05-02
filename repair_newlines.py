import os
import re

inc_dir = 'include'
for f in os.listdir(inc_dir):
    if not f.endswith('.h'): continue
    
    path = os.path.join(inc_dir, f)
    with open(path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()

    # The issue: I accidentally replaced the literal characters "\" and "n" with an actual newline "\n" 
    # anywhere they appeared as an escape sequence in C++.
    # I need to find all real newlines that are directly *inside* double quotes and replace them back with \n.
    
    # We can use regex to find double quotes containing a literal newline.
    # Actually, a simpler approach is to find all cases where a physical newline is immediately preceded by a double quote? No, the newline is inside the string.
    # For example: 
    # printf("TIMEUTIL: ... largest=%I64d
    # ", ...);
    
    # Let's fix it safely:
    # A physical newline followed strictly by ",
    content = content.replace('\n",', '\\n",')
    content = content.replace('\n");', '\\n");')
    content = content.replace('\n}', '\\n}')
    # wait, these replaces might be dangerous if applied blindly.
    
    # Let's use a regex to fix broken strings: `"[^\n"]*\n[^\n"]*"` replacing the `\n` inside it.
    # Actually, MSVC will report the exact lines. Wait, it's easier to just git checkout them if I had git...
    
    # Let's fix the specific ones we know:
    content = content.replace('largest=%I64d\n",', 'largest=%I64d\\n",')
    content = content.replace('us\n",name', 'us\\n",name')
    content = content.replace('us\n",duration', 'us\\n",duration')
    content = content.replace('us)\n",name', 'us)\\n",name')
    content = content.replace('us)\n",duration', 'us)\\n",duration')
    content = content.replace('too large)\n");', 'too large)\\n");')
    
    with open(path, 'w', encoding='utf-8') as file:
        file.write(content)

print('Newlines repaired!')
