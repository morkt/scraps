# compact directory tree, moving all files into current directory

import os
import sys

if len(sys.argv) < 2:
    print("usage: {0} <directory>".format(sys.argv[0]))
    sys.exit()

root_dir = sys.argv[1]

def scan_tree(path, cur_name):
    with os.scandir(path) as it:
        for entry in it:
            full_name = os.path.join(path, entry.name)
            if entry.is_dir():
                scan_tree(full_name, cur_name+entry.name+'_')
            elif cur_name:
                new_name = cur_name+entry.name
                print(new_name)
                os.rename(full_name, os.path.join(root_dir, new_name))

scan_tree(root_dir, '')
