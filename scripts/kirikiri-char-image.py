# build "Hikoukigumo no Mukougawa" character images from parts [kirikiri engine]

import sys
import codecs
import re
import os

if len(sys.argv) < 4:
    print ("usage: {0} [-e ENCODING] CHAR_DEF.txt BASE-ID OVERLAY-ID".format (sys.argv[0]))
    sys.exit()

CONVERT = 'convert.exe'

sys.stdout = codecs.getwriter("utf-8")(sys.stdout.detach())
argn = 1
encoding = 'shift-jis'
if sys.argv[argn] == '-e':
    encoding = sys.argv[argn+1]
    argn += 2
txt_name = sys.argv[argn]
base_id = sys.argv[argn+1]
argn += 2

name = re.sub (r'\..*$', '', txt_name)

imgset = []
layers = {}

with open (txt_name, 'r', encoding=encoding) as txt:
    first_line = txt.readline()
    if first_line.startswith(u'\uFEFF'):
        first_line = first_line[1:]
    keys = first_line.split ("\t")
    columns = {}
    for i in range (len (keys)):
        columns[keys[i]] = i
    if not 'width' in columns or not 'height' in columns:
        raise Exception('incorrect image definition')
    row = txt.readline().split ("\t")
    width = int(row[columns['width']])
    height = int(row[columns['height']])
    for line in txt:
        row = line.split ("\t")
        img = {}
        for key in keys:
            img[key] = row[columns[key]]
        imgset.append (img)
        if '0' == img['#layer_type']:
            layers[img['layer_id']] = img

if not base_id in layers:
    raise Exception ("base {0} not found".format (base_id))
base = layers[base_id]
target_name = '{0}_{1}'.format (name, base['layer_id'])
base_name = target_name+ '.png'

cmd = [ '-size', '{0}x{1}'.format (width, height), 'xc:none' ]
x = int(base['left'])
y = int(base['top'])
geometry = '{0:+d}{1:+d}'.format (x, y)
cmd.extend ([base_name, '-geometry', geometry, '-composite'])

for overlay_id in sys.argv[argn:]:
    if not overlay_id in layers:
        raise Exception ("overlay {0} not found".format (overlay_id))
    overlay = layers[overlay_id]

    overlay_name = '{0}_{1}.png'.format (name, overlay['layer_id'])
    x = int(overlay['left'])
    y = int(overlay['top'])
    alpha = int(overlay['opacity'])
    geometry = '{0:+d}{1:+d}'.format (x, y)
    cmd.extend ([overlay_name, '-geometry', geometry, '-composite'])
#    if alpha != 255:
#        cmd.extend (['-channel', 'A', '-evaluate', 'set', '{0}%'.format(int(alpha*100/255))])
    target_name += '+{0}'.format (overlay['layer_id']);

target_name += '.png';
cmd.append (target_name)
print (' '.join (['convert']+cmd))
os.system (' '.join ([CONVERT] + cmd));
