import numpy as np
import matplotlib.pyplot as plt
from matplotlib import ticker, cm

data = np.fromfile('./gain_map.raw', dtype=np.float64).reshape((4028, 6034))
print(data)

x_step = 6034//40
y_step = 4028//30

data_plt = data[::y_step, ::x_step]
x = np.arange(data_plt.shape[1])
y = np.arange(data_plt.shape[0])

X, Y = np.meshgrid(x, y)

fig = plt.figure(0)
ax = fig.add_subplot(111)
ax.set_title('sigma 35 1.4 A102 nikon vignette demo')
cs = ax.contourf(X, Y, data_plt, cmap=cm.get_cmap('coolwarm'))
cbar = fig.colorbar(cs)
plt.show()