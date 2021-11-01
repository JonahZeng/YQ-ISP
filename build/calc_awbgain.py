import numpy as np

rawdata = np.fromfile('_ATM8691_lsc_dump.raw', np.uint16).reshape((4028, 6034))

roi1 = rawdata[2326:2466, 2822:2958]

roi2 = rawdata[2326:2466, 3074:3208]

roi1_r = roi1[::2, ::2]
roi1_gr = roi1[::2, 1::2]
roi1_gb = roi1[1::2, ::2]
roi1_b = roi1[1::2, 1::2]

roi2_r = roi2[::2, ::2]
roi2_gr = roi2[::2, 1::2]
roi2_gb = roi2[1::2, ::2]
roi2_b = roi2[1::2, 1::2]

roi1_g_mean = (np.mean(roi1_gr)+np.mean(roi1_gb))/2
rgain = roi1_g_mean/np.mean(roi1_r)
bgain = roi1_g_mean/np.mean(roi1_b)
print(rgain*1024, bgain*1024)

roi2_g_mean = (np.mean(roi2_gr)+np.mean(roi2_gb))/2
rgain = roi2_g_mean/np.mean(roi2_r)
bgain = roi2_g_mean/np.mean(roi2_b)
print(rgain*1024, bgain*1024)