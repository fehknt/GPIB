import os

inc_dir = 'include'
for f in os.listdir(inc_dir):
    if not f.endswith('.h'): continue
    path = os.path.join(inc_dir, f)
    with open(path, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()
    
    # Fix the corrupted backslash-n from the previous buggy script
    content = content.replace('\\\\n', '\\n')
    
    # Make sure math.h and float.h are present before typedefs.h
    if '<math.h>' not in content and 'typedefs.h' in content:
        content = content.replace('#include \"typedefs.h\"', '#include <math.h>\\n#include <float.h>\\n#include \"typedefs.h\"')
        
    with open(path, 'w', encoding='utf-8') as file:
        file.write(content)

print('Headers repaired and math/float dependencies injected!')
