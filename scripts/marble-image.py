# combine images for games based on marble engine

import sys
import os

output_dir = 'r'
overlay_dir = 'ovl'

if len(sys.argv) < 2:
    print ("usage: {0} _BGSET.S".format (sys.argv[0]))
    sys.exit()

bgset = None
with open (sys.argv[1], encoding='shift-jis') as f:
    bgset = f.read().split ("\0")

os.makedirs (output_dir, exist_ok=True)

i = 0;
while i < len(bgset):
    name = bgset[i]
    i += 1
    if not name.startswith (':'):
        raise Exception ("{0}: invalid operator".format (name))
    if ':END' == name:
        break
    name = name[1:] + '.png'
    base_name = bgset[i] + '.png'
    i += 1
    overlays = []
    while i < len(bgset) and not bgset[i].startswith (':'):
        ovl_name, x, y = bgset[i].split (',')
        if ovl_name != 'blank':
            overlays.append (overlay_dir+'/'+ovl_name+'.png')
        i += 1

    if overlays and os.path.isfile (base_name):
        output_name = output_dir + '/' + name
        cmd = 'pngblend -o {0} {1} {2}'.format (output_name, base_name, ' '.join (overlays))
        print (cmd)
        os.system (cmd)
