# combine character images for games based on marble engine

import sys
import os

output_dir = 'r'
CONVERT = 'C:/usr/ImageMagick/convert.exe';

if len(sys.argv) < 2:
    print ("usage: {0} _SPRSET.S".format (sys.argv[0]))
    sys.exit()

sprset = None
with open (sys.argv[1], encoding='shift-jis') as f:
    sprset = f.read().split ("\0")

os.makedirs (output_dir, exist_ok=True)

i = 0
while i < len(sprset):
    name = sprset[i]
    i += 1
    if not name:
        continue
    if not name.startswith (':'):
        raise Exception ("\"{0}\": invalid operator".format (name))
    if ':END' == name:
        break
    name = name[1:] + '.png'
    base_name = sprset[i] + '.png'
    i += 1
    overlays = []
    while i < len(sprset) and not sprset[i].startswith (':'):
        ovl_name, x, y = sprset[i].split (',')
        if ovl_name != 'blank':
            geometry = '{0:+}{1:+}'.format (x, y)
            overlays.extend ([ovl_name+'.png', '-geometry', geometry, '-composite'])
        i += 1

    if overlays and os.path.isfile (base_name):
        output_name = output_dir + '/' + name
        args = ' '.join ([base_name] + overlays + [output_name])
        print ('convert '+args)
        os.system (CONVERT+' '+args)
