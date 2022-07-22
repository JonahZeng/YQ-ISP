import numpy as np

rawdata = np.fromfile('X3A2S_Office1_rgbir_rm_dump0.raw', np.uint8).reshape((1280, 1920))

roi1 = rawdata[788:864, 1156:1216]


roi1_b = roi1[0::2, 0::2]
roi1_gb = roi1[0::2, 1::2]
roi1_gr = roi1[1::2, 0::2]
roi1_r = roi1[1::2, 1::2]


roi1_g_mean = (np.mean(roi1_gr)+np.mean(roi1_gb))/2
rgain = roi1_g_mean/np.mean(roi1_r)
bgain = roi1_g_mean/np.mean(roi1_b)
print(rgain*1024, bgain*1024)
