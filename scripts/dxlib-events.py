# combine event images for DXLIB-based games based on _BGSET and _VIEW

import struct
import sys
import os.path
import re
import codecs
from enum import Enum

output_dir = 'r'
CONVERT = 'C:/usr/ImageMagick/convert.exe'
PNGBLEND = 'pngblend.exe'

if len(sys.argv) < 3:
    print("usage: {0} _BGSET _VIEW".format(sys.argv[0]))
    sys.exit()

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())

def read_def(filename):
    with open (filename, 'rb') as sprset:
        sprset.seek (4)
        offset = struct.unpack ('<I', sprset.read(4))[0]
        sprset.seek (0x10+offset)
        return sprset.read()

class State(Enum):
    init = 0
    defspr = 1

def parse_def(data):
    lines = data.split(bytes(1))

    state = State.init
    charList = []
    current_char = {}

    for line in lines:
        text = line.decode('sjis')
        if not text or text.startswith(':'):
            continue
        if state == State.init:
            if text == '{':
                state = State.defspr
                current_char = {}
            continue
        if text == '}':
            state = State.init
            if current_char:
                charList.append (current_char)
                current_char = {}
            continue
        pair = text.split(':')
        if not pair or not pair[1]:
            continue
        key = pair[0]
        values = pair[1].split(',')
        if key in current_char:
            current_char[key].append (values)
        else:
            current_char[key] = [ values ]
    return charList

bgList = parse_def(read_def(sys.argv[1]))
viewList = parse_def(read_def(sys.argv[2]))

os.makedirs (output_dir, exist_ok=True)

view_re = re.compile('^\d+$')

usedViews = {}

for view in viewList:
    for key in view:
        if not view_re.match(key):
            continue
        values = view[key][0]
        if not values or len(values) < 2:
            continue
        base_name = values[0]
        if view_re.match(base_name):
            continue
        views = []
        for name in values[1:]:
            if not name or len(name) < 2:
                continue
            if name.startswith('#'):
                name = name[1:]
            usedViews[name] = 1

for bg in bgList:
    if not all (key in bg for key in ('PREFIX', 'BODY', 'PARTS')):
        continue
    base_target = bg['PREFIX'][0][0]
    base_name = bg['PREFIX'][0][1]
    for body in bg['BODY']:
        body_target = base_target + body[0]
        body_name = base_name + body[1] + '.png'
        if not os.path.isfile (body_name):
            continue
        for face in bg['PARTS']:
            view_name = body_target + face[0]
            if not view_name in usedViews:
                continue
            face_target = output_dir + '/' + body_target + face[0] + '.png'
            face_name = base_name + face[1] + '.png'
            if os.path.isfile (face_target) or not os.path.isfile (face_name):
                continue
            if len(face) > 3:
                x = int(face[2])
                y = int(face[3])
            else:
                x = 0
                y = 0

            if x == 0 and y == 0:
                cmd = PNGBLEND
                args = ' '.join (['-o', face_target, body_name, face_name])
            else:
                cmd = CONVERT
                geometry = '{0:+}{1:+}'.format (x, y)
                args = ' '.join ([body_name, face_name, '-geometry', geometry, '-composite', face_target])
            print ('blending '+args)
            sys.stdout.flush()
            os.system (cmd+' '+args)
