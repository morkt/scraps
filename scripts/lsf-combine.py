# blend images described in LSF file

import sys
import codecs
import os
import struct

if len(sys.argv) < 5:
    print ("usage: {0} FILE.lsf OUTPUT BASE OVERLAY".format (sys.argv[0]))
    sys.exit()

CONVERT = 'C:\\usr\\ImageMagick\\convert.exe'

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())
encoding = 'shift-jis'
lsf_name = sys.argv[1]
out_name = sys.argv[2]
base_id = sys.argv[3]

sprites = {}
with open (lsf_name, 'rb') as lsf:
    if lsf.read (3) != b'LSF':
        raise Exception ("{0}: invalid LSF file".format (lsf_name))
    lsf.seek (7, 1)
    count = struct.unpack ('<H', lsf.read (2))[0]
    lsf.seek (0x10, 1)
    while count > 0:
        name   = struct.unpack ('@128s', lsf.read (0x80))[0].split (b'\0', 1)[0].decode (encoding)
        left   = struct.unpack ('<i', lsf.read (4))[0]
        top    = struct.unpack ('<i', lsf.read (4))[0]
        right  = struct.unpack ('<i', lsf.read (4))[0]
        bottom = struct.unpack ('<i', lsf.read (4))[0]
        lsf.seek (0x14, 1)
        count = count - 1
        sprites[name] = [ left, top, right, bottom ]

if base_id not in sprites:
    raise Exception ("{0}: base {1} not found".format (lsf_name, base_id))

width = 0
height = 0
ovl_cmd = []

for ovl_id in sys.argv[3:]:
    if ovl_id not in sprites:
        raise Exception ("{0}: overlay {1} not found".format (lsf_name, ovl_id))
    ovl = sprites[ovl_id]
    x = ovl[0]
    y = ovl[1]
    ovl_cmd.extend ([ ovl_id+'.png', '-geometry', '{0+}{1+}'.format (x, y), '-composite' ])
    if ovl[2] > width:
        width = ovl[2]
    if ovl[3] > height:
        height = ovl[3]

cmd = [ '-size', '{0}x{1}'.format (width, height), 'xc:none' ]
cmd.extend (ovl_cmd)
cmd.append (out_name)

args = ' '.join (cmd)
print ('convert ' + args)
os.system (CONVERT+' '+args)
