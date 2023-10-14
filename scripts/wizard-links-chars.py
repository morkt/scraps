# combine character images for Wizard Links

import struct
import sys
import os.path
import codecs
from enum import Enum

output_dir = 'r'

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
    defspr = 1

state = State.init
charList = []
current_char = {}

for line in lines:
    text = line.decode('sjis')
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

for char in charList:
    if not all (key in char for key in ('PREFIX', 'BODY', 'FACE')):
        continue
    base_target = char['PREFIX'][0][0]
    base_name = char['PREFIX'][0][1]
    for body in char['BODY']:
        body_target = base_target + body[0]
        body_name = base_name + body[1] + '.png'
        if not os.path.isfile (body_name):
            continue
        for face in char['FACE']:
            face_target = output_dir + '/' + body_target + face[0] + '.png'
            face_name = base_name + face[1] + '.png'
            if not os.path.isfile (face_name):
                continue
            cmd = 'pngblend -o {0} {1} {2}'.format (face_target, body_name, face_name)
            os.system (cmd)
