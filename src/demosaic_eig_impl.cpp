#include <cstdint>
#include <cmath>
#include "common.h"
#include "demosaic_hw.h"

#define CLIP3(x, min_, max_) ((x < min_) ? min_ : ((x > max_) ? max_ : x))
#define MAX2(a, b) (a > b ? a : b)
#define MIN2(a, b) (a > b ? b : a)

static pixel_bayer_type get_pixel_bayer_type(uint32_t y, uint32_t x, bayer_type_t by)
{
    uint32_t pos = ((y % 2) << 1) + (x % 2);

    pixel_bayer_type type[4];
    if (by == RGGB)
    {
        type[0] = PIX_R;
        type[1] = PIX_GR;
        type[2] = PIX_GB;
        type[3] = PIX_B;
    }
    else if (by == GRBG)
    {
        type[0] = PIX_GR;
        type[1] = PIX_R;
        type[2] = PIX_B;
        type[3] = PIX_GB;
    }
    else if (by == GBRG)
    {
        type[0] = PIX_GB;
        type[1] = PIX_B;
        type[2] = PIX_R;
        type[3] = PIX_GR;
    }
    else if (by == BGGR)
    {
        type[0] = PIX_B;
        type[1] = PIX_GB;
        type[2] = PIX_GR;
        type[3] = PIX_R;
    }

    return type[pos];
}

static int32_t estimate_hor_cd(uint16_t *head, int32_t *filter)
{
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)head[i - 2] * filter[i];
    }
    return ret;
}

static int32_t estimate_ver_cd(uint16_t *head, int32_t *filter, uint32_t width)
{
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)head[(i - 2)*(int32_t)width] * filter[i];
    }
    return ret;
}

static int32_t estimate_dia1_cd(uint16_t *head, int32_t *filter, uint32_t width)
{
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)head[(2 - i) * (int32_t)width + (i - 2)] * filter[i];
    }
    return ret;
}

static int32_t estimate_dia2_cd(uint16_t *head, int32_t *filter, uint32_t width)
{
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)head[(i - 2) * (int32_t)width + (i - 2)] * filter[i];
    }
    return ret;
}

static int32_t calc_east_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                            uint32_t x_, uint32_t y_)
{
    int32_t filter[5] = {1, -3, 4, -3, 1};
    double alpha = 3.75 / 6.0;

    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[y_ * (xsize + EXT_X) + x_ + 2]);
    int32_t d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t cd0 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_, filter);
    int32_t cd1 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ + 1, filter);
    int32_t cd2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ + 2, filter);
    int32_t cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line0_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_n1_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_p1_eig = ci_e + (int32_t)(alpha * cd_e);
    int32_t e_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return e_eig;
}

static int32_t calc_west_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                            uint32_t x_, uint32_t y_)
{
    int32_t filter[5] = {1, -3, 4, -3, 1};
    double alpha = 3.75 / 6.0;

    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[y_ * (xsize + EXT_X) + x_ - 2]);
    int32_t d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t cd0 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_, filter);
    int32_t cd1 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ - 1, filter);
    int32_t cd2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ - 2, filter);
    int32_t cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line0_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_n1_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_p1_eig = ci_e + (int32_t)(alpha * cd_e);
    int32_t w_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return w_eig;
}

static int32_t calc_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                             uint32_t x_, uint32_t y_)
{
    int32_t filter[5] = {1, -3, 4, -3, 1};
    double alpha = 3.75 / 6.0;

    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd1 = estimate_ver_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line0_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_n1_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_p1_eig = ci_e + (int32_t)(alpha * cd_e);
    int32_t n_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return n_eig;
}

static int32_t calc_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                             uint32_t x_, uint32_t y_)
{
    int32_t filter[5] = {1, -3, 4, -3, 1};
    double alpha = 3.75 / 6.0;

    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd1 = estimate_ver_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
    int32_t cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line0_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_n1_eig = ci_e + (int32_t)(alpha * cd_e);

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    int32_t line_p1_eig = ci_e + (int32_t)(alpha * cd_e);
    int32_t s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t calc_east_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 2]);
    int32_t d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);

    int32_t line0_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 3) * (xsize + EXT_X) + x_ + 2]);

    int32_t line_n1_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 3]);

    int32_t line_p1_eig = (d1 + d2 + 1) / 2;
    int32_t  s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t calc_west_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 2]);
    int32_t d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1]);

    int32_t line0_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 3) * (xsize + EXT_X) + x_ - 2]);

    int32_t line_n1_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 3]);

    int32_t line_p1_eig = (d1 + d2 + 1) / 2;
    int32_t s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t calc_west_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 2]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);

    int32_t line0_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[(y_-1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ +1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_-1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 3]);

    int32_t line_n1_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_+1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 3) * (xsize + EXT_X) + x_ - 2]);

    int32_t line_p1_eig = (d1 + d2 + 1) / 2;
    int32_t s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t calc_east_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 2]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);

    int32_t line0_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[(y_-1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ +1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_+1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 3]);

    int32_t line_n1_eig = (d1 + d2 + 1) / 2;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_+1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 3) * (xsize + EXT_X) + x_ + 2]);

    int32_t line_p1_eig = (d1 + d2 + 1) / 2;
    int32_t s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static uint16_t estimate_final_g_for_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                       uint32_t x_, uint32_t y_, int32_t east_eig, int32_t west_eig, int32_t south_eig, int32_t north_eig)
{
    double T = 1.9;
    int32_t T_cd = 6;

    int32_t p = indata[y_ * (xsize + EXT_X) + x_];
    int32_t p_right1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
    int32_t p_right2 = indata[y_ * (xsize + EXT_X) + x_ + 2];
    int32_t p_right3 = indata[y_ * (xsize + EXT_X) + x_ + 3];
    int32_t p_left1 = indata[y_ * (xsize + EXT_X) + x_ - 1];
    int32_t p_left2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
    int32_t p_left3 = indata[y_ * (xsize + EXT_X) + x_ - 3];
    int32_t p_up1 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
    int32_t p_up2 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
    int32_t p_up3 = indata[(y_ - 3) * (xsize + EXT_X) + x_];
    int32_t p_down1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
    int32_t p_down2 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
    int32_t p_down3 = indata[(y_ + 3) * (xsize + EXT_X) + x_];
    int32_t g_east_estimate = p_right1 + (p - p_right2) / 2 + (p_left1 - 2 * p_right1 + p_right3) / 8;
    int32_t g_west_estimate = p_left1 + (p - p_left2) / 2 + (p_left3 - 2 * p_left1 + p_right1) / 8;
    int32_t g_north_estimate = p_up1 + (p - p_up2) / 2 + (p_up3 - 2 * p_up1 + p_down1) / 8;
    int32_t g_south_estimate = p_down1 + (p - p_down2) / 2 + (p_up1 - 2 * p_down1 + p_down3) / 8;

    double g_hor_estimate = 0.0;
    if(west_eig + east_eig == 0)
    {
         g_hor_estimate = (g_east_estimate + g_west_estimate + 1) / 2;
    }
    else{
        g_hor_estimate = (g_east_estimate * west_eig + g_west_estimate * east_eig) / (west_eig + east_eig);
    }
    double g_ver_estimate = 0.0;
    if(south_eig + north_eig == 0)
    {
        g_ver_estimate = (g_north_estimate + g_south_estimate + 1) / 2;
    }
    else{
        g_ver_estimate = (g_north_estimate * south_eig + g_south_estimate * north_eig) / (south_eig + north_eig);
    }
    // double g_dia_estimate = (g_north_estimate / north_eig + g_south_estimate / south_eig + g_east_estimate / east_eig + g_west_estimate / west_eig) /
    //                         (1.0 / north_eig + 1.0 / south_eig + 1.0 / east_eig + 1.0 / west_eig);
    double g_dia_estimate = 0.0;
    if(west_eig + east_eig + south_eig + north_eig == 0)
    {
        g_dia_estimate = (g_hor_estimate + g_ver_estimate + 1) / 2;
    }
    else
    {
        g_dia_estimate = (g_hor_estimate * (south_eig + north_eig) + g_ver_estimate * (west_eig + east_eig)) / (west_eig + east_eig + south_eig + north_eig);
    }

    g_hor_estimate = CLIP3(g_hor_estimate, 0.0, 16383.0);
    g_ver_estimate = CLIP3(g_ver_estimate, 0.0, 16383.0);
    g_dia_estimate = CLIP3(g_dia_estimate, 0.0, 16383.0);

    double hor_eig = east_eig + west_eig;
    double ver_eig = north_eig + south_eig;
    double E = 0.0;
    double min_eig = MIN2(hor_eig, ver_eig);
    if(abs(min_eig - 0.0) < 0.000001)
    {
        E = T + 1.0;
    }
    else
    {
        E = MAX2(hor_eig, ver_eig) / MIN2(hor_eig, ver_eig);
    }

    uint16_t out = 0;

    if (E < T)
    {
        int32_t filter[5] = {1, -3, 4, -3, 1};
        int32_t cd_h_0 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_, filter);
        int32_t cd_h_n2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ - 2, filter);
        int32_t cd_h_p2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ + 2, filter);

        int32_t cd_v_0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_v_n2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_v_p2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);

        int32_t omg_cd_h = abs(cd_h_n2 - cd_h_0) + abs(cd_h_p2 - cd_h_0);
        int32_t omg_cd_v = abs(cd_v_n2 - cd_v_0) + abs(cd_v_p2 - cd_v_0);

        int32_t omg_diff = abs(omg_cd_h - omg_cd_v);
        if (omg_diff < T_cd)
        {
            out = (uint16_t)g_dia_estimate;
        }
        else if (omg_cd_h < omg_cd_v)
        {
            out = (uint16_t)g_hor_estimate;
        }
        else
        {
            out = (uint16_t)g_ver_estimate;
        }
    }
    else
    {
        if (hor_eig > ver_eig)
        {
            out = (uint16_t)g_ver_estimate;
        }
        else
        {
            out = (uint16_t)g_hor_estimate;
        }
    }
    return out;
}

static uint16_t estimate_final_rb_for_br(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                      uint32_t x_, uint32_t y_, int32_t en_eig, int32_t wn_eig, int32_t es_eig, int32_t ws_eig)
{
    double T = 1.9;
    int32_t T_cd = 6;

    int32_t p = indata[y_ * (xsize + EXT_X) + x_];
    int32_t p_en1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
    int32_t p_en2 = indata[(y_ - 2) * (xsize + EXT_X) + x_ + 2];
    int32_t p_en3 = indata[(y_ - 3) * (xsize + EXT_X) + x_ + 3];

    int32_t p_wn1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
    int32_t p_wn2 = indata[(y_ - 2) * (xsize + EXT_X) + x_ - 2];
    int32_t p_wn3 = indata[(y_ - 3) * (xsize + EXT_X) + x_ - 3];

    int32_t p_es1 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
    int32_t p_es2 = indata[(y_ + 2) * (xsize + EXT_X) + x_ + 2];
    int32_t p_es3 = indata[(y_ + 3) * (xsize + EXT_X) + x_ + 3];

    int32_t p_ws1 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
    int32_t p_ws2 = indata[(y_ + 2) * (xsize + EXT_X) + x_ - 2];
    int32_t p_ws3 = indata[(y_ + 3) * (xsize + EXT_X) + x_ - 3];

    int32_t r_en_estimate = p_en1 + (p - p_en2) / 2 + (p_ws1 - 2 * p_en1 + p_en3) / 8;
    int32_t r_wn_estimate = p_wn1 + (p - p_wn2) / 2 + (p_es1 - 2 * p_wn1 + p_wn3) / 8;
    int32_t r_es_estimate = p_es1 + (p - p_es2) / 2 + (p_wn1 - 2 * p_es1 + p_es3) / 8;
    int32_t r_ws_estimate = p_ws1 + (p - p_ws2) / 2 + (p_en1 - 2 * p_ws1 + p_ws3) / 8;

    double r_dia1_estimate = 0.0;
    if(ws_eig + en_eig == 0)
    {
        r_dia1_estimate = (r_en_estimate + r_ws_estimate + 1) / 2;
    }
    else{
        r_dia1_estimate = (r_en_estimate * ws_eig + r_ws_estimate * en_eig) / (ws_eig + en_eig);
    }
    double r_dia2_estimate = 0.0;
    if(es_eig + wn_eig == 0){
        r_dia2_estimate = (r_wn_estimate + r_es_estimate  + 1) / 2;
    }
    else{
        r_dia2_estimate = (r_wn_estimate * es_eig + r_es_estimate * wn_eig) / (es_eig + wn_eig);
    }
    // double r_dia_estimate = (r_en_estimate / en_eig + r_wn_estimate / wn_eig + r_es_estimate / es_eig + r_ws_estimate / ws_eig) /
    //                         (1.0 / en_eig + 1.0 / wn_eig + 1.0 / es_eig + 1.0 / ws_eig);
     double r_dia_estimate = 0.0;
     if(ws_eig + en_eig + es_eig + wn_eig == 0)
     {
        r_dia_estimate = (r_dia1_estimate + r_dia2_estimate + 1) / 2;
     }
     else{
        r_dia_estimate = (r_dia1_estimate *(es_eig + wn_eig) + r_dia2_estimate * (ws_eig + en_eig)) / (ws_eig + en_eig + es_eig + wn_eig);
     }

    r_dia1_estimate = CLIP3(r_dia1_estimate, 0.0, 16383.0);
    r_dia2_estimate = CLIP3(r_dia2_estimate, 0.0, 16383.0);
    r_dia_estimate = CLIP3(r_dia_estimate, 0.0, 16383.0);

    double dia1_eig = ws_eig + en_eig;
    double dia2_eig = es_eig + wn_eig;
    double E = 0.0;
    double min_eig = MIN2(dia1_eig, dia2_eig);
    if(abs(min_eig - 0.0) < 0.000001)
    {
        E = T + 1.0;
    }
    else
    {
        E = MAX2(dia1_eig, dia2_eig) / MIN2(dia1_eig, dia2_eig);
    }

    uint16_t out = 0;

    if (E < T)
    {
        int32_t filter[5] = {1, -3, 4, -3, 1};
        int32_t cd_dia1_0 = estimate_dia1_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_dia1_n2 = estimate_dia1_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ - 2, filter, xsize + EXT_X);
        int32_t cd_dia1_p2 = estimate_dia1_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ + 2, filter, xsize + EXT_X);

        int32_t cd_dia2_0 = estimate_dia2_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_dia2_n2 = estimate_dia2_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ - 2, filter, xsize + EXT_X);
        int32_t cd_dia2_p2 = estimate_dia2_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ + 2, filter, xsize + EXT_X);

        int32_t omg_cd_dia1 = abs(cd_dia1_n2 - cd_dia1_0) + abs(cd_dia1_p2 - cd_dia1_0);
        int32_t omg_cd_dia2 = abs(cd_dia2_n2 - cd_dia2_0) + abs(cd_dia2_p2 - cd_dia2_0);

        int32_t omg_diff = abs(omg_cd_dia1 - omg_cd_dia2);
        if (omg_diff < T_cd)
        {
            out = (uint16_t)r_dia_estimate;
        }
        else if (omg_cd_dia1 < omg_cd_dia2)
        {
            out = (uint16_t)r_dia1_estimate;
        }
        else
        {
            out = (uint16_t)r_dia2_estimate;
        }
    }
    else
    {
        if (dia1_eig > dia2_eig)
        {
            out = (uint16_t)r_dia2_estimate;
        }
        else
        {
            out = (uint16_t)r_dia1_estimate;
        }
    }
    return out;
}

static void estimate_r_up_down_for_Gr(uint16_t *indata, uint16_t *r_out, uint32_t x_, uint32_t y_, uint32_t row, uint32_t col,
                                        uint32_t xsize, uint32_t EXT_X, int32_t* r_up, int32_t* r_down)
{
    if(row >= 1)
    {
        *r_up = r_out[(row - 1) * xsize + col];
    }
    else
    {
        int32_t r_wn = indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1];
        int32_t r_en = indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1];
        int32_t r_ws = indata[y_ * (xsize + EXT_X) + x_ - 1];
        int32_t r_es = indata[y_ * (xsize + EXT_X) + x_ + 1];
        int32_t grad_d1 = abs(r_en - r_ws);
        int32_t grad_d2 = abs(r_wn - r_es);
        int32_t r_d1 = (r_en + r_ws) / 2;
        int32_t r_d2 = (r_wn + r_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *r_up = r_wn;
        }
        else
        {
            *r_up = (r_d1 * grad_d2 + r_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t r_wn = indata[y_ * (xsize + EXT_X) + x_ - 1];
        int32_t r_en = indata[y_ * (xsize + EXT_X) + x_ + 1];
        int32_t r_ws = indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1];
        int32_t r_es = indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1];
        int32_t grad_d1 = abs(r_en - r_ws);
        int32_t grad_d2 = abs(r_wn - r_es);
        int32_t r_d1 = (r_en + r_ws) / 2;
        int32_t r_d2 = (r_wn + r_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *r_down = r_wn;
        }
        else
        {
            *r_down = (r_d1 * grad_d2 + r_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}

static void estimate_b_left_right_for_Gr(uint16_t *indata, uint16_t *b_out, uint32_t x_, uint32_t y_, 
uint32_t row, uint32_t col, uint32_t xsize, uint32_t EXT_X, int32_t* b_left, int32_t* b_right)
{
    if(col >= 1)
    {
        *b_left = b_out[row * xsize + col - 1];
    }
    else
    {
        int32_t b_wn = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2];
        int32_t b_en = indata[(y_ - 1) * (xsize + EXT_X) + x_];
        int32_t b_ws = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 2];
        int32_t b_es = indata[(y_ + 1) * (xsize + EXT_X) + x_];
        int32_t grad_d1 = abs(b_en - b_ws);
        int32_t grad_d2 = abs(b_wn - b_es);
        int32_t b_d1 = (b_en + b_ws) / 2;
        int32_t b_d2 = (b_wn + b_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *b_left = b_wn;
        }
        else
        {
            *b_left = (b_d1 * grad_d2 + b_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t b_wn = indata[(y_ - 1) * (xsize + EXT_X) + x_];
        int32_t b_en = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2];
        int32_t b_ws = indata[(y_ + 1) * (xsize + EXT_X) + x_];
        int32_t b_es = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 2];
        int32_t grad_d1 = abs(b_en - b_ws);
        int32_t grad_d2 = abs(b_wn - b_es);
        int32_t b_d1 = (b_en + b_ws) / 2;
        int32_t b_d2 = (b_wn + b_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *b_right = b_wn;
        }
        else
        {
            *b_right = (b_d1 * grad_d2 + b_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}


static void estimate_r_left_right_for_Gb(uint16_t *indata, uint16_t *r_out, uint32_t x_, uint32_t y_, 
    uint32_t row, uint32_t col, uint32_t xsize, uint32_t EXT_X, int32_t* r_left, int32_t* r_right)
{
    if(col >= 1)
    {
        *r_left = r_out[row * xsize + col - 1];
    }
    else
    {
        int32_t r_wn = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2];
        int32_t r_en = indata[(y_ - 1) * (xsize + EXT_X) + x_];
        int32_t r_ws = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 2];
        int32_t r_es = indata[(y_ + 1) * (xsize + EXT_X) + x_];
        int32_t grad_d1 = abs(r_en - r_ws);
        int32_t grad_d2 = abs(r_wn - r_es);
        int32_t r_d1 = (r_en + r_ws) / 2;
        int32_t r_d2 = (r_wn + r_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *r_left = r_wn;
        }
        else
        {
            *r_left = (r_d1 * grad_d2 + r_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t r_wn = indata[(y_ - 1) * (xsize + EXT_X) + x_];
        int32_t r_en = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2];
        int32_t r_ws = indata[(y_ + 1) * (xsize + EXT_X) + x_];
        int32_t r_es = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 2];
        int32_t grad_d1 = abs(r_en - r_ws);
        int32_t grad_d2 = abs(r_wn - r_es);
        int32_t r_d1 = (r_en + r_ws) / 2;
        int32_t r_d2 = (r_wn + r_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *r_right = r_wn;
        }
        else
        {
            *r_right = (r_d1 * grad_d2 + r_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}


static void estimate_b_up_down_for_Gb(uint16_t *indata, uint16_t *b_out, uint32_t x_, uint32_t y_, 
    uint32_t row, uint32_t col, uint32_t xsize,  uint32_t EXT_X, int32_t* b_up, int32_t* b_down)
{
    if(row >= 1)
    {
        *b_up = b_out[(row - 1) * xsize + col];
    }
    else
    {
        int32_t b_wn = indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1];
        int32_t b_en = indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1];
        int32_t b_ws = indata[y_ * (xsize + EXT_X) + x_ - 1];
        int32_t b_es = indata[y_ * (xsize + EXT_X) + x_ + 1];
        int32_t grad_d1 = abs(b_en - b_ws);
        int32_t grad_d2 = abs(b_wn - b_es);
        int32_t b_d1 = (b_en + b_ws) / 2;
        int32_t b_d2 = (b_wn + b_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *b_up = b_wn;
        }
        else
        {
            *b_up = (b_d1 * grad_d2 + b_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t b_wn = indata[y_ * (xsize + EXT_X) + x_ - 1];
        int32_t b_en = indata[y_ * (xsize + EXT_X) + x_ + 1];
        int32_t b_ws = indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1];
        int32_t b_es = indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1];
        int32_t grad_d1 = abs(b_en - b_ws);
        int32_t grad_d2 = abs(b_wn - b_es);
        int32_t b_d1 = (b_en + b_ws) / 2;
        int32_t b_d2 = (b_wn + b_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *b_down = b_wn;
        }
        else
        {
            *b_down = (b_d1 * grad_d2 + b_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}

static uint16_t estimate_final_rb_for_g(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
            uint32_t x_, uint32_t y_, double east_eig, double west_eig, double south_eig, double north_eig,
            int32_t p_up, int32_t p_down, int32_t p_left, int32_t p_right)
{
    double T = 1.9;
    int32_t T_cd = 6;

    int32_t p = indata[y_ * (xsize + EXT_X) + x_];
    int32_t p_right1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
    int32_t p_right2 = indata[y_ * (xsize + EXT_X) + x_ + 2];
    int32_t p_right3 = indata[y_ * (xsize + EXT_X) + x_ + 3];
    int32_t p_left1 = indata[y_ * (xsize + EXT_X) + x_ - 1];
    int32_t p_left2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
    int32_t p_left3 = indata[y_ * (xsize + EXT_X) + x_ - 3];
    int32_t p_up1 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
    int32_t p_up2 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
    int32_t p_up3 = indata[(y_ - 3) * (xsize + EXT_X) + x_];
    int32_t p_down1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
    int32_t p_down2 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
    int32_t p_down3 = indata[(y_ + 3) * (xsize + EXT_X) + x_];
    int32_t east_estimate = p_right + (p - p_right2) / 2 + (p_left1 - 2 * p_right1 + p_right3) / 8;
    int32_t west_estimate = p_left + (p - p_left2) / 2 + (p_left3 - 2 * p_left1 + p_right1) / 8;
    int32_t north_estimate = p_up + (p - p_up2) / 2 + (p_up3 - 2 * p_up1 + p_down1) / 8;
    int32_t south_estimate = p_down + (p - p_down2) / 2 + (p_up1 - 2 * p_down1 + p_down3) / 8;

    double hor_estimate = 0.0;
    if(west_eig + east_eig == 0)
    {
        hor_estimate = (east_estimate + west_estimate + 1) / 2;
    }
    else{
        hor_estimate = (east_estimate * west_eig + west_estimate * east_eig) / (west_eig + east_eig);
    }
    double ver_estimate = 0.0;
    if(south_eig + north_eig == 0)
    {
        ver_estimate = (north_estimate + south_estimate + 1) / 2;
    }
    else{
        ver_estimate = (north_estimate * south_eig + south_estimate * north_eig) / (south_eig + north_eig);
    }
    // double dia_estimate = (north_estimate / north_eig + south_estimate / south_eig + east_estimate / east_eig + west_estimate / west_eig) /
    //                         (1.0 / north_eig + 1.0 / south_eig + 1.0 / east_eig + 1.0 / west_eig);
    double dia_estimate = 0.0;
    if(west_eig + east_eig + south_eig + north_eig == 0)
    {
        dia_estimate = (hor_estimate + ver_estimate + 1) / 2;
    }
    else{
        dia_estimate = (hor_estimate * (south_eig + north_eig) + ver_estimate * (west_eig + east_eig)) / (west_eig + east_eig + south_eig + north_eig);
    }

    hor_estimate = CLIP3(hor_estimate, 0.0, 16383.0);
    ver_estimate = CLIP3(ver_estimate, 0.0, 16383.0);
    dia_estimate = CLIP3(dia_estimate, 0.0, 16383.0);

    double hor_eig = east_eig + west_eig;
    double ver_eig = north_eig + south_eig;
    double E = 0.0;
    double min_eig = MIN2(hor_eig, ver_eig);
    if(abs(min_eig - 0.0) < 0.000001)
    {
        E = T + 1.0;
    }
    else
    {
        E = MAX2(hor_eig, ver_eig) / MIN2(hor_eig, ver_eig);
    }

    uint16_t out = 0;

    if (E < T)
    {
        int32_t filter[5] = {1, -3, 4, -3, 1};
        int32_t cd_h_0 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_, filter);
        int32_t cd_h_n2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ - 2, filter);
        int32_t cd_h_p2 = estimate_hor_cd(indata + y_ * (xsize + EXT_X) + x_ + 2, filter);

        int32_t cd_v_0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_v_n2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);
        int32_t cd_v_p2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_, filter, xsize + EXT_X);

        int32_t omg_cd_h = abs(cd_h_n2 - cd_h_0) + abs(cd_h_p2 - cd_h_0);
        int32_t omg_cd_v = abs(cd_v_n2 - cd_v_0) + abs(cd_v_p2 - cd_v_0);

        int32_t omg_diff = abs(omg_cd_h - omg_cd_v);
        if (omg_diff < T_cd)
        {
            out = (uint16_t)dia_estimate;
        }
        else if (omg_cd_h < omg_cd_v)
        {
            out = (uint16_t)hor_estimate;
        }
        else
        {
            out = (uint16_t)ver_estimate;
        }
    }
    else
    {
        if (hor_eig > ver_eig)
        {
            out = (uint16_t)ver_estimate;
        }
        else
        {
            out = (uint16_t)hor_estimate;
        }
    }
    return out;
}

static int32_t calc_patch_similarity(uint16_t* indata, int32_t pch_top, int32_t pch_left,  uint16_t (*src_val)[3], int32_t width)
{
    int32_t wei[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
    int32_t similar = 0;
    for(int32_t j=0; j<3; j++)
    {
        for(int32_t i=0; i<3; i++)
        {
            int32_t diff = abs((int32_t)src_val[j][i] - (int32_t)indata[(pch_top+j)*width + pch_left + i]);
            similar += diff * wei[j*3+i]; //14 + 2 + 4
        }
    }
    return similar / 16;
}

static inline int32_t flip_board(int32_t x, int32_t xsize)
{
    if(x<0)
    {
        return abs(x);
    }
    else if(x>xsize-1)
    {
        return (2*xsize - 2 - x);
    }
    else
        return x;
}

void demosaic_hw_core_eig(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                             uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg)
{
    pixel_bayer_type pix_type;
    uint32_t x_, y_;

    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;

            if (pix_type == PIX_R)
            {
                int32_t east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t en_eig = calc_east_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t wn_eig = calc_west_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t es_eig = calc_east_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t ws_eig = calc_west_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];

                g_out[row * xsize + col] = estimate_final_g_for_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, east_eig,
                                                                   west_eig, south_eig, north_eig);
                b_out[row * xsize + col] = estimate_final_rb_for_br(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    en_eig, wn_eig, es_eig, ws_eig);
            }
            else if (pix_type == PIX_GR)
            {
                int32_t east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];

                int32_t r_up = 0;
                int32_t r_down = 0;
                int32_t r_left = indata[y_ * (xsize + EXT_X) + x_ - 1];
                int32_t r_right = indata[y_ * (xsize + EXT_X) + x_ + 1];
                estimate_r_up_down_for_Gr(indata, r_out, x_, y_, row, col, xsize,  EXT_X, &r_up, &r_down);
                r_out[row * xsize + col] = estimate_final_rb_for_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    east_eig, west_eig, south_eig, north_eig, r_up, r_down, r_left, r_right);

                int32_t b_left = 0;
                int32_t b_right = 0;
                int32_t b_up = indata[(y_ - 1)* (xsize + EXT_X) + x_];
                int32_t b_down = indata[(y_ + 1)* (xsize + EXT_X) + x_];
                estimate_b_left_right_for_Gr(indata, b_out, x_, y_, row, col, xsize,  EXT_X, &b_left, &b_right);
                b_out[row * xsize + col] = estimate_final_rb_for_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    east_eig, west_eig, south_eig, north_eig, b_up, b_down, b_left, b_right);
            }
            else if (pix_type == PIX_GB)
            {
                int32_t east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];

                int32_t r_left = 0;
                int32_t r_right = 0;
                int32_t r_up = indata[(y_-1)*(xsize+EXT_X) + x_];
                int32_t r_down = indata[(y_+1)*(xsize+EXT_X) + x_];
                estimate_r_left_right_for_Gb(indata, r_out, x_, y_, row, col, xsize,  EXT_X, &r_left, &r_right);

                r_out[row * xsize + col] = estimate_final_rb_for_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    east_eig, west_eig, south_eig, north_eig, r_up, r_down, r_left, r_right);

                int32_t b_up = 0;
                int32_t b_down = 0;
                int32_t b_left = indata[y_ * (xsize + EXT_X) + x_ - 1];
                int32_t b_right = indata[y_ * (xsize + EXT_X) + x_ + 1];
                estimate_b_up_down_for_Gb(indata, b_out, x_, y_, row, col, xsize,  EXT_X, &b_up, &b_down);

                b_out[row * xsize + col] = estimate_final_rb_for_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    east_eig, west_eig, south_eig, north_eig, b_up, b_down, b_left, b_right);
            }
            else if (pix_type == PIX_B)
            {
                int32_t east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                int32_t en_eig = calc_east_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t wn_eig = calc_west_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t es_eig = calc_east_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t ws_eig = calc_west_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                b_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];

                g_out[row * xsize + col] = estimate_final_g_for_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    east_eig, west_eig, south_eig, north_eig);
                r_out[row * xsize + col] = estimate_final_rb_for_br(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    en_eig, wn_eig, es_eig, ws_eig);
            }

            r_out[row * xsize + col] = (r_out[row * xsize + col] > 16383) ? 16383 : r_out[row * xsize + col];
            g_out[row * xsize + col] = (g_out[row * xsize + col] > 16383) ? 16383 : g_out[row * xsize + col];
            b_out[row * xsize + col] = (b_out[row * xsize + col] > 16383) ? 16383 : b_out[row * xsize + col];
        }
    }

    //two pass
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;
            uint16_t src_val[3][3] = {0};

            if (pix_type == PIX_R)
            {
                // NLM b
                src_val[0][0] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = b_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                double total_similar = 0.0;
                double total_pix_b = 0.0;
                for (int32_t j = 0; j < 4; j++)
                {
                    for (int32_t i = 0; i < 4; i++)
                    {
                        int32_t pch_top = (int32_t)row - 4 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;

                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);

                        total_similar += exp_similar;
                        uint16_t know_b = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_b += know_b * exp_similar;
                    }
                }
                total_pix_b += b_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_b = total_pix_b / total_similar;
                mean_pix_b = CLIP3(mean_pix_b, 0.0, 16383.0);
                b_out[row * xsize + col] = (uint16_t)mean_pix_b;

                //NLM Gr
                src_val[0][0] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = r_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = g_out[row * xsize + col];
                src_val[1][2] = r_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                total_similar = 0.0;
                double total_pix_g = 0.0;
                for (int32_t j = 0; j < 3; j++)
                {
                    for (int32_t i = 0; i < 4; i++)
                    {
                        int32_t pch_top = (int32_t)row - 3 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;

                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 16);

                        total_similar += exp_similar;
                        uint16_t know_g = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_g += know_g * exp_similar;
                    }
                }
                total_pix_g += g_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_g = total_pix_g / total_similar;
                mean_pix_g = CLIP3(mean_pix_g, 0.0, 16383.0);
                g_out[row * xsize + col] = (uint16_t)mean_pix_g;
            }
            else if (pix_type == PIX_GR)
            {
                // NLM b
                src_val[0][0] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = b_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                double total_similar = 0.0;
                double total_pix_b = 0.0;
                for (int32_t j = 0; j < 4; j++)
                {
                    for (int32_t i = 0; i < 3; i++)
                    {
                        int32_t pch_top = (int32_t)row - 4 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 3 + 2 * i + EXT_X/2;

                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);

                        total_similar += exp_similar;
                        uint16_t know_b = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_b += know_b * exp_similar;
                    }
                }
                total_pix_b += b_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_b = total_pix_b / total_similar;
                mean_pix_b = CLIP3(mean_pix_b, 0.0, 16383.0);
                b_out[row * xsize + col] = (uint16_t)mean_pix_b;


                // NLM r
                src_val[0][0] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = r_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];

                total_similar = 0.0;
                double total_pix_r = 0.0;
                for (int32_t j = 0; j < 3; j++)
                {
                    for (int32_t i = 0; i < 4; i++)
                    {
                        int32_t pch_top = (int32_t)row - 3 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;
                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);
                        total_similar += exp_similar;
                        uint16_t know_r = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_r += know_r * exp_similar;
                    }
                }
                total_pix_r += r_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_r = total_pix_r / total_similar;
                mean_pix_r = CLIP3(mean_pix_r, 0.0, 16383.0);
                r_out[row * xsize + col] = (uint16_t)mean_pix_r;
            }
            else if (pix_type == PIX_GB)
            {
                // NLM b
                src_val[0][0] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = b_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                double total_similar = 0.0;
                double total_pix_b = 0.0;
                for (int32_t j = 0; j < 3; j++)
                {
                    for (int32_t i = 0; i < 4; i++)
                    {
                        int32_t pch_top = (int32_t)row - 3 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;

                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);

                        total_similar += exp_similar;
                        uint16_t know_b = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_b += know_b * exp_similar;
                    }
                }
                total_pix_b += b_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_b = total_pix_b / total_similar;
                mean_pix_b = CLIP3(mean_pix_b, 0.0, 16383.0);
                b_out[row * xsize + col] = (uint16_t)mean_pix_b;


                // NLM r
                src_val[0][0] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = r_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];

                total_similar = 0.0;
                double total_pix_r = 0.0;
                for (int32_t j = 0; j < 4; j++)
                {
                    for (int32_t i = 0; i < 3; i++)
                    {
                        int32_t pch_top = (int32_t)row - 4 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 3 + 2 * i + EXT_X/2;
                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);
                        total_similar += exp_similar;
                        uint16_t know_r = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_r += know_r * exp_similar;
                    }
                }
                total_pix_r += r_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_r = total_pix_r / total_similar;
                mean_pix_r = CLIP3(mean_pix_r, 0.0, 16383.0);
                r_out[row * xsize + col] = (uint16_t)mean_pix_r;
            }
            else if (pix_type == PIX_B)
            {
                // NLM r
                src_val[0][0] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[0][1] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                src_val[0][2] = b_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[1][0] = g_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[1][1] = r_out[row * xsize + col];
                src_val[1][2] = g_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                src_val[2][0] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                src_val[2][1] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                src_val[2][2] = b_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];

                double total_similar = 0.0;
                double total_pix_r = 0.0;
                for (int32_t j = 0; j < 4; j++)
                {
                    for (int32_t i = 0; i < 4; i++)
                    {
                        int32_t pch_top = (int32_t)row - 4 + 2 * j + EXT_Y/2;
                        int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;

                        int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                        double exp_similar = 1.0 / exp(differ / 64);
                        total_similar += exp_similar;
                        uint16_t know_r = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                        total_pix_r += know_r * exp_similar;
                    }
                }
                total_pix_r += r_out[row * xsize + col];
                total_similar += 1.0;
                double mean_pix_r = total_pix_r / total_similar;
                mean_pix_r = CLIP3(mean_pix_r, 0.0, 16383.0);
                r_out[row * xsize + col] = (uint16_t)mean_pix_r;

                //NLM Gb
                // src_val[0][0] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                // src_val[0][1] = r_out[flip_board((int32_t)row - 1, ysize) * xsize + col];
                // src_val[0][2] = g_out[flip_board((int32_t)row - 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                // src_val[1][0] = b_out[row * xsize + flip_board((int32_t)col - 1, xsize)];
                // src_val[1][1] = g_out[row * xsize + col];
                // src_val[1][2] = b_out[row * xsize + flip_board((int32_t)col + 1, xsize)];
                // src_val[2][0] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col - 1, xsize)];
                // src_val[2][1] = r_out[flip_board((int32_t)row + 1, ysize) * xsize + col];
                // src_val[2][2] = g_out[flip_board((int32_t)row + 1, ysize) * xsize + flip_board((int32_t)col + 1, xsize)];
                // total_similar = 0.0;
                // double total_pix_g = 0.0;
                // for (int32_t j = 0; j < 3; j++)
                // {
                //     for (int32_t i = 0; i < 4; i++)
                //     {
                //         int32_t pch_top = (int32_t)row - 3 + 2 * j + EXT_Y/2;
                //         int32_t pch_left = (int32_t)col - 4 + 2 * i + EXT_X/2;

                //         int32_t differ = calc_patch_similarity(indata, pch_top, pch_left, src_val, xsize + EXT_X); //s15 u14
                        
                //         double exp_similar = 1.0 / exp(differ / 64);

                //         total_similar += exp_similar;
                //         uint16_t know_g = indata[(pch_top+1)*(xsize+EXT_X)+pch_left+1];
                //         total_pix_g += know_g * exp_similar;
                //     }
                // }
                // total_pix_g += g_out[row * xsize + col];
                // total_similar += 1.0;
                // double mean_pix_g = total_pix_g / total_similar;
                // mean_pix_g = CLIP3(mean_pix_g, 0.0, 16383.0);
                // g_out[row * xsize + col] = (uint16_t)mean_pix_g;
            }
        }
    }
}