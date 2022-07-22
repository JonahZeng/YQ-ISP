import numpy as np

img_data = np.fromfile('X3A2S_Office1.raw', np.uint16).reshape((1280, 1920))
img_data = img_data.newbyteorder()
print(img_data.max())
img_data = img_data.astype(np.int16) - 128
img_data[img_data<0] = 0
roi = img_data[416:428, 1784:1796]
b_mean = (roi[0::4, 0::4].mean() + roi[2::4, 2::4].mean())/2
r_mean = (roi[0::4, 2::4].mean() + roi[2::4, 0::4].mean())/2
g_mean = (roi[0::2, 1::2].mean() + roi[1::2, 0::2].mean())/2
ir_mean = roi[1::2, 1::2].mean()
print('b mean {0}'.format(b_mean))
print('r mean {0}'.format(r_mean))
print('g mean {0}'.format(g_mean))
print('ir mean {0}'.format(ir_mean))