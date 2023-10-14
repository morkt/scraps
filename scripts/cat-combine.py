# combine CatSystem CG images based on defintions from cglist.dat

import os
import sys
import struct
import shutil

output_dir = 'r'

if len(sys.argv) < 2:
    print("usage: {0} cglist.dat".format(sys.argv[0]))
    sys.exit()

cglist = {}
with open (sys.argv[1], 'rb') as cg:
    cg.seek(8)
    count = struct.unpack ('<i', cg.read(4))[0]
    for i in range(count):
        size = struct.unpack ('<i', cg.read(4))[0]
        base_name = cg.read(0x40).partition(b'\0')[0].decode('sjis')
        if not base_name:
            break
        parts_count = struct.unpack ('<i', cg.read(4))[0]
        parts = []
        for j in range(parts_count):
            part_name = cg.read(0x40).partition(b'\0')[0].decode('sjis')
            if not part_name:
                continue
            parts.append (part_name)
        cglist[base_name] = parts

for cg_variants in cglist.values():
    for parts_str in cg_variants:
        parts = parts_str.split(',')
        if not parts:
            continue
        base_name = parts[0]
        if len(parts) == 1:
            base_name = base_name + '.png'
            if os.path.isfile (base_name):
                shutil.copy (base_name, os.path.join (output_dir, base_name))
            else:
                print("missing file {0}".format (base_name))
            continue
        base_name = base_name + '_'
        target_name = base_name
        part_list = []
        for part in parts[1:]:
            target_name = target_name + part
            if part != '0':
                part_name = base_name + part + '.png'
                if os.path.isfile(part_name):
                    part_list.append (part_name)
                else:
                    print("missing file {0}".format (part_name))
            base_name = base_name + '0'

        target_name = os.path.join (output_dir, target_name + '.png')
        if os.path.isfile (target_name):
            continue
        if len(part_list) > 1:
            cmd = [ 'pngblend', '-o', target_name ]
            cmd.extend (part_list)
            os.system(' '.join (cmd))
        else:
            shutil.copy (part_list[0], target_name)
