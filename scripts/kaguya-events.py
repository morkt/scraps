# combine character images for games based on kaguya engine

import sys
import glob
import os
import re

out_dir = 'r'
parts_dir = 'parts'

if not os.path.isdir (out_dir): 
    os.mkdir (out_dir)

cg_re = re.compile ('^(cg\d+)(.*)\.png$')

for base in glob.glob (u'cg*.png'):
    match = cg_re.match (base)
    if match:
        base_name = match.group(1) + match.group(2)
        part_glob = parts_dir + '/' + match.group(1) + '*.png'
        for part in glob.glob (part_glob):
            if os.path.isfile (part):
                match = cg_re.match (os.path.basename (part))
                if match:
                    out = out_dir + '/' + base_name + '_' + match.group(2) + '.png'
                    os.system ('pngblend -o {0} {1} {2}'.format (out, base, part))
