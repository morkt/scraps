# restore event CG for Tetsu to Ra

import os
import re
import sys
import shutil

output_dir = 'r'
CONVERT = r'C:\usr\ImageMagick\convert.exe'
restore_pubic_hair = True

if len(sys.argv) < 2:
    print("usage: {0} CGLIST.TXT".format(sys.argv[0]))
    sys.exit()

images = []
with open (sys.argv[1], encoding='shift-jis') as cglist:
    images = [ line.rstrip('\n') for line in cglist ]

os.makedirs (output_dir, exist_ok=True)

base_re = re.compile ('^((.+)[0-9][0-9]Z)(.+)')

for event in images:
    name = event + '.png'
    target_name = output_dir + '/' + name
    if os.path.isfile (target_name):
        continue
    print (name)
    match = base_re.match (event)
    if not match:
        if os.path.isfile (name):
            cg_ib = event + 'ib.png'
            if restore_pubic_hair and os.path.isfile (cg_ib):
                os.system (' '.join ([CONVERT, name, cg_ib, '-composite', target_name]))
            else:
                shutil.copy (name, target_name)
        else:
            print (name+': not found')
    else:
        base = match.group(1)
        parts = match.group(3)
        tpl = '#' * len(parts)
        cg_base = base+tpl+'.png'
        cg_ib = base+tpl+'ib.png'
        if not os.path.isfile (cg_base):
            print (cg_base+': not found')
            continue
        layers = []
        if tpl != parts:
            base = match.group(2)
            for i in range(len(parts)):
                num = parts[i]
                if num != '#':
                    cg_part = base+'##Y'+('#' * i)+num+('#' * (len(parts)-i-1))+'.png'
                    if os.path.isfile (cg_part):
                        layers.append (cg_part)
        if restore_pubic_hair and os.path.isfile (cg_ib):
            layers.append (cg_ib)
        if len(layers) == 0:
            shutil.copy (cg_base, target_name)
        else:
            cmd = [CONVERT, cg_base]
            for layer in layers:
                cmd.extend ([layer, '-composite'])
            cmd.append (target_name)
            os.system (' '.join (cmd))
