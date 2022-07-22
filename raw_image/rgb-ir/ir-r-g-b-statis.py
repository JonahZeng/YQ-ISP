import os
import numpy as np

raws = [x for x in os.listdir() if x.endswith('.raw')]

for img in raws:
    print(img)
    img_data = np.fromfile(img, np.uint16).reshape((1280, 1920))
    img_data = img_data.newbyteorder()
    img_data = img_data.astype(np.int16) - 32
    img_data[img_data<0] = 0
    b_mean = (img_data[0::4, 0::4].mean() + img_data[2::4, 2::4].mean())/2
    r_mean = (img_data[0::4, 2::4].mean() + img_data[2::4, 0::4].mean())/2
    g_mean = (img_data[0::2, 1::2].mean() + img_data[1::2, 0::2].mean())/2
    ir_mean = img_data[1::2, 1::2].mean()
    print('b mean {0}'.format(b_mean))
    print('r mean {0}'.format(r_mean))
    print('g mean {0}'.format(g_mean))
    print('ir mean {0}'.format(ir_mean))