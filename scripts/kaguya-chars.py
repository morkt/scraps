# combine character images for games based on kaguya engine

import sys
import glob
import os
import re

out_dir = 'r'

if not os.path.isdir (out_dir): 
    os.mkdir (out_dir)

ovl_re = re.compile ('^(cg.*_)モ(\d+)甲\.png$')

for ovl in glob.glob (u'cg*_モ*甲.png'):
    match = ovl_re.match (ovl)
    if match:
        base = match.group(1)+match.group(2)+'.png'
        if os.path.isfile (base):
            out = out_dir + '/' + base
            os.system ('pngblend -o {0} {1} {2}'.format (out, base, ovl))
