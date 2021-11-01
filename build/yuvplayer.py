import numpy as np
import cv2 as cv

yuv = np.fromfile('_ATM8636_rgb2yuv_dump.yuv', np.uint8).reshape((4016, 6016, 3))
print(yuv.shape)
bgr = cv.cvtColor(yuv, cv.COLOR_YUV2BGR)

cv.imwrite('_ATM8636_rgb2yuv_dump.jpg', bgr)