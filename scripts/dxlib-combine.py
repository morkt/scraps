# combine character images for DxLib engine based on definitions from _SPRSET/_BGSET

import struct
import sys
import os.path
import codecs
import shutil
from enum import Enum

output_dir = 'r'
CONVERT = 'C:/usr/ImageMagick/convert.exe'
PNGBLEND = 'pngblend.exe'
force_overwrite = False

if len(sys.argv) < 2:
    print("usage: {0} _SPRSET".format(sys.argv[0]))
    sys.exit()

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())

data = None
with open (sys.argv[1], 'rb') as sprset:
    sprset.seek (4)
    offset = struct.unpack ('<I', sprset.read(4))[0]
    sprset.seek (0x10+offset)
    data = sprset.read()

lines = data.split(bytes(1))

class State(Enum):
    init = 0
    body = 1
    overlay = 2

state = State.init
charList = []
current_char = {}

for line in lines:
    text = line.decode('sjis')
    if text.startswith('#'):
        if current_char:
            charList.append (current_char)
        text = text[1:]
        if not text:
            state = State.init
            continue
        current_char = { 'TARGET': text }
        state = State.body
    elif current_char:
        if state == State.body:
            current_char['BODY'] = text
            state = State.overlay
        elif state == State.overlay:
            current_char['OVERLAY'] = text.split(',')

os.makedirs (output_dir, exist_ok=True)

for char in charList:
    output_name = output_dir + '/' + char['TARGET']+'.png'
    if not force_overwrite and os.path.isfile (output_name):
        continue
    body_name = char['BODY']+'.png'
    if not os.path.isfile (body_name):
        sys.stdout.flush()
        continue
    if not char['OVERLAY'] or char['OVERLAY'][0] == 'BLANK':
        print ('copy '+body_name+' -> '+output_name)
        sys.stdout.flush()
        shutil.copy (body_name, output_name)
        continue

    overlay_name = char['OVERLAY'][0] + '.png'
    if not os.path.isfile (overlay_name):
        continue
    args = ''
    cmd = ''
    x = int (char['OVERLAY'][1])
    y = int (char['OVERLAY'][2])
    if x == 0 and y == 0:
        cmd = PNGBLEND
        args = ' '.join (['-o', output_name, body_name, overlay_name])
    else:
        cmd = CONVERT
        geometry = '{0:+}{1:+}'.format (x, y)
        args = ' '.join ([body_name, overlay_name, '-geometry', geometry, '-composite', output_name])
    print ('blending '+args)
    sys.stdout.flush()
    os.system (cmd+' '+args)
