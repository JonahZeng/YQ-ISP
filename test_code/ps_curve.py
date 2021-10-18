import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backend_bases import MouseButton
from scipy.interpolate import make_interp_spline, CubicSpline
from matplotlib.ticker import MultipleLocator, AutoMinorLocator

class PointBrowser:
    def __init__(self, dots, all_dots):
        self.pick_line = dots
        self.show_line = all_dots
        self.lastind = 0
        self.left_button_pressed = False

        # self.text = ax.text(0.05, 0.95, 'selected: none',
        #                     transform=ax.transAxes, va='top')
        self.selected, = ax.plot([xs[0]], [ys[0]], 'o', ms=12, alpha=0.4,
                                 color='yellow', visible=False)

    def on_button_press(self, event):
        if self.lastind is None:
            return
        if event.button != MouseButton.LEFT:
            return
        self.left_button_pressed = True

    def on_button_release(self, event):
        if self.lastind is None:
            return
        if event.button != MouseButton.LEFT:
            return
        self.left_button_pressed = False

    def on_mouse_move(self, event):
        if self.lastind is None:
            return
        if self.left_button_pressed == True:
            xy_data = self.pick_line.get_xydata()
            xy_data[self.lastind][0] = event.xdata
            xy_data[self.lastind][1] = event.ydata
            self.pick_line.set_xdata(xy_data[:, 0])
            self.pick_line.set_ydata(xy_data[:, 1])
            b = CubicSpline(xy_data[:, 0], xy_data[:, 1])

            all_x_data = self.show_line.get_xdata()
            all_y_data = np.clip(b(all_x_data), 0, 16383)
            self.show_line.set_ydata(all_y_data)
            self.update()


    def on_pick(self, event):
        if event.artist != self.pick_line:
            return True

        N = len(event.ind)
        if not N:
            return True

        self.lastind = event.ind[0]
        self.update()

    def update(self):
        if self.lastind is None:
            return
        dataind = self.lastind
        self.selected.set_visible(True)
        self.selected.set_data(self.pick_line.get_xdata()[dataind], self.pick_line.get_ydata()[dataind])
        # self.text.set_text('selected: %d' % dataind)
        fig.canvas.draw()

    def on_key_press(self, event):
        if event.key == 'ctrl+n':
            np.set_printoptions(suppress = True, precision=0)
            all_x_data = self.show_line.get_xdata()
            all_y_data = self.show_line.get_ydata()
            print(all_x_data)
            print('==============================')
            print(all_y_data)

if __name__ == '__main__':
    xs = np.array([0, 2048, 4096, 8192, 12288, 14336, 16383], dtype=np.float64)
    ys = np.array([0, 2048, 4096, 8192, 12288, 14336, 16383], dtype=np.float64)

    all_x = np.arange(0, 16383, step=64)
    all_y = all_x.copy()

    fig, ax = plt.subplots(1, 1)
    ax.set_title('click on point to pick')
    ax.grid(True)
    ax.set_xlim([-1024, 18000])
    ax.set_ylim([-1024, 18000])
    ax.xaxis.set_major_locator(MultipleLocator(2048))
    ax.xaxis.set_minor_locator(MultipleLocator(256))
    ax.yaxis.set_major_locator(MultipleLocator(2048))
    ax.yaxis.set_minor_locator(MultipleLocator(256))
    dots, = ax.plot(xs, ys, 'o', picker=True, pickradius=5)
    all_dots, = ax.plot(all_x, all_y)

    browser = PointBrowser(dots, all_dots)

    fig.canvas.mpl_connect('pick_event', browser.on_pick)
    fig.canvas.mpl_connect('button_press_event', browser.on_button_press)
    fig.canvas.mpl_connect('button_release_event', browser.on_button_release)
    fig.canvas.mpl_connect('motion_notify_event', browser.on_mouse_move)
    fig.canvas.mpl_connect('key_press_event', browser.on_key_press)

    plt.show()