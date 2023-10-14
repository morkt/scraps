# blend images described in Kiss' Scene file (Chijoku no Gakuen)

import sys
import codecs
import os
import glob

if len(sys.argv) < 2:
    print ("usage: {0} Scene.dat".format (sys.argv[0]))
    sys.exit()

output_dir = 'r'
CONVERT = 'C:\\usr\\ImageMagick\\convert.exe'

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())
encoding = 'shift-jis'

for scene_name in glob.glob(sys.argv[1]):
    scene_dict = {}

    with open (scene_name, 'r', encoding=encoding) as scene_dat:
        for line in scene_dat:
            row = line.split (',')
            name = row[1]
            if '_moz' in name:
                continue
            id = int(row[0])
            layer = { 'name': name, 'prio': int(row[2]), 'x': int(row[3]), 'y': int(row[4]) }
            if id in scene_dict:
                scene_dict[id].append (layer)
            else:
                scene_dict[id] = [ layer ]

    for scene_id, layers in scene_dict.items():
        layers = sorted (layers, key=lambda item: item['prio'])
        name_parts = layers[0]['name'].split('_')
        if name_parts[0] == 'EV':
            base_name = name_parts[0] + '_' + name_parts[1]
        else:
            base_name = name_parts[0]
        out_name = '{0}/{1}_{2:03}.png'.format (output_dir, base_name, scene_id)
        if os.path.isfile(out_name):
            continue
        base_x = layers[0]['x']
        base_y = layers[0]['y']
        cmd = [ layers[0]['name'] ]
        for layer in layers[1:]:
            geometry = '{0:+d}{1:+d}'.format (layer['x'] - base_x, layer['y'] - base_y)
            cmd.extend ([ layer['name'], '-geometry', geometry, '-composite' ])
        cmd.append (out_name)
        args = ' '.join (cmd)
        print ('convert ' + args, flush = True)
        os.system (CONVERT+' '+args)
