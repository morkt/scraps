# restore event images for MONSTER PARK

import struct
import os

with open ('EVINFO.BIN', 'rb') as ev:
    count = struct.unpack ('<i', ev.read (4))[0]
    for i in range (count):
        target_name = ev.read (0x10).partition (b'\0')[0].decode('sjis')
        if not target_name:
            break
        if not target_name.startswith ('M'):
            ev.seek (0x44, 1)
            continue
        base_name = ev.read (0x10).partition (b'\0')[0].decode('sjis')
        diff_name = ev.read (0x10).partition (b'\0')[0].decode('sjis')
        target_name += '.png'
        base_name   += '.png'
        diff_name   = 'd/' + diff_name + '.png'
        ev.seek (0x24, 1)
        if os.path.isfile (base_name) and os.path.isfile (diff_name):
            cmd = [ 'pngblend', '-o', target_name, base_name, diff_name ]
            os.system (' '.join (cmd))
