# combine character images for Lune games (_filemacro)

import struct
import codecs
import re
import sys
import os

if len(sys.argv) < 2:
    print("usage: {0} _filemacro".format(sys.argv[0]))
    sys.exit()

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())

pngblend = 'pngblend.exe'

with open (sys.argv[1], encoding="sjis") as filemacro:
    while True:
        line = filemacro.readline()
        if not line or line == '[END]':
            break
        match = re.match(r'^([^=]+)=(.*)', line)
        if match:
            out_name = match.group(1)+'.png'
            parts = match.group(2).split('/')
            if not os.path.isfile(parts[0]+'.png') or os.path.isfile(out_name):
                continue
            if len(parts) == 1:
                os.link(parts[0]+'.png', out_name)
            else:
                args = [ pngblend, '-o', out_name ]
                args += list(map(lambda x: x+'.png', parts))
                cmd = ' '.join(args)
                os.system (cmd)
