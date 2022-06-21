#include "demosaic_hw.h"
#include "pipe_register.h"
#include <memory>

#define CLIP3(x, min_, max_) ((x < min_) ? min_ : ((x > max_) ? max_ : x))
#define MAX2(a, b) (a > b ? a : b)
#define MIN2(a, b) (a > b ? b : a)

demosaic_hw::demosaic_hw(uint32_t inpins, uint32_t outpins, const char *inst_name) : hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    demosaic_reg = new demosaic_reg_t;
}

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
    else
    {
        type[0] = PIX_B;
        type[1] = PIX_GB;
        type[2] = PIX_GR;
        type[3] = PIX_R;
    }

    return type[pos];
}

static void demosaic_hw_core_bilinear(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                                      uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg)
{
    pixel_bayer_type pix_type;
    uint32_t x_, y_;
    uint16_t r_0, r_1, r_2, r_3;
    uint16_t g_0, g_1, g_2, g_3;
    uint16_t b_0, b_1, b_2, b_3;
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;
            if (pix_type == PIX_R)
            {
                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                g_out[row * xsize + col] = (g_0 + g_1 + g_2 + g_3 + 2) / 4; // 16383 *4 = 65532

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                b_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                b_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (b_0 + b_1 + b_2 + b_3 + 2) / 4;
            }
            else if (pix_type == PIX_GR)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (r_0 + r_1 + 1) / 2;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                b_out[row * xsize + col] = (b_0 + b_1 + 1) / 2;
            }
            else if (pix_type == PIX_GB)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (b_0 + b_1 + 1) / 2;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                r_out[row * xsize + col] = (r_0 + r_1 + 1) / 2;
            }
            else if (pix_type == PIX_B)
            {
                b_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                g_out[row * xsize + col] = (g_0 + g_1 + g_2 + g_3 + 2) / 4; // 16383 *4 = 65532

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                r_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                r_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (r_0 + r_1 + r_2 + r_3 + 2) / 4;
            }
        }
    }
}

static void demosaic_hw_core_imp_linear(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                                        uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg)
{
    pixel_bayer_type pix_type;
    uint32_t x_, y_;
    uint32_t r_0, r_1, r_2, r_3, r_c;
    uint32_t g_0, g_1, g_2, g_3, g_4, g_5, g_6, g_7, g_c;
    uint32_t b_0, b_1, b_2, b_3, b_c;
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;
            if (pix_type == PIX_R)
            {
                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];

                r_c = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                r_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                r_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_out[row * xsize + col] = (2 * (g_0 + g_1 + g_2 + g_3) + 4 * r_c - r_0 - r_1 - r_2 - r_3 + 4) >> 3;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                b_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                b_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (4 * (b_0 + b_1 + b_2 + b_3) + 12 * r_c - 3 * (r_0 + r_1 + r_2 + r_3) + 8) >> 4;
            }
            else if (pix_type == PIX_GR)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];

                g_c = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                g_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                g_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                g_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_4 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                g_5 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                g_6 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                g_7 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (8 * (r_0 + r_1) + 10 * g_c + g_0 + g_1 - 2 * (g_2 + g_3 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                b_out[row * xsize + col] = (8 * (b_0 + b_1) + 10 * g_c + g_2 + g_3 - 2 * (g_0 + g_1 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;
            }
            else if (pix_type == PIX_GB)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];

                g_c = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                g_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                g_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                g_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_4 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                g_5 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                g_6 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                g_7 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];

                b_out[row * xsize + col] = (8 * (b_0 + b_1 + 1) + 10 * g_c + g_0 + g_1 - 2 * (g_2 + g_3 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                r_out[row * xsize + col] = (8 * (r_0 + r_1) + 10 * g_c + g_2 + g_3 - 2 * (g_0 + g_1 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;
            }
            else if (pix_type == PIX_B)
            {
                b_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];

                b_c = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                b_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                b_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];

                g_out[row * xsize + col] = (2 * (g_0 + g_1 + g_2 + g_3) + 4 * b_c - (b_0 + b_1 + b_2 + b_3) + 4) >> 3;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                r_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                r_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (4 * (r_0 + r_1 + r_2 + r_3) + 12 * b_c - 3 * (b_0 + b_1 + b_2 + b_3) + 8) >> 4;
            }

            r_out[row * xsize + col] = (r_out[row * xsize + col] > 16383) ? 16383 : r_out[row * xsize + col];
            g_out[row * xsize + col] = (g_out[row * xsize + col] > 16383) ? 16383 : g_out[row * xsize + col];
            b_out[row * xsize + col] = (b_out[row * xsize + col] > 16383) ? 16383 : b_out[row * xsize + col];
        }
    }
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

static double calc_east_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
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

    double line0_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_n1_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_p1_eig = ci_e + alpha * cd_e;
    double e_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return e_eig;
}

static double calc_west_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
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

    double line0_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_n1_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_, filter);
    cd1 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 1, filter);
    cd2 = estimate_hor_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 2, filter);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_p1_eig = ci_e + alpha * cd_e;
    double w_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return w_eig;
}

static double calc_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
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

    double line0_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_n1_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ - 1) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ - 2) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_p1_eig = ci_e + alpha * cd_e;
    double n_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return n_eig;
}

static double calc_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
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

    double line0_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ - 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_n1_eig = ci_e + alpha * cd_e;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);
    ci_e = (d1 + d2) / 2;

    cd0 = estimate_ver_cd(indata + y_ * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd1 = estimate_ver_cd(indata + (y_ + 1) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd2 = estimate_ver_cd(indata + (y_ + 2) * (xsize + EXT_X) + x_ + 1, filter, xsize + EXT_X);
    cd_e = (abs(cd0 + cd1) + abs(cd1 + cd2)) / 2;

    double line_p1_eig = ci_e + alpha * cd_e;
    double s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static double calc_east_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 2]);
    int32_t d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1]);

    double line0_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 3) * (xsize + EXT_X) + x_ + 2]);

    double line_n1_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ + 3]);

    double line_p1_eig = (d1 + d2) / 2.0;
    double s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static double calc_west_north_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 2]);
    int32_t d2 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1]);

    double line0_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 3) * (xsize + EXT_X) + x_ - 2]);

    double line_n1_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ - 2) * (xsize + EXT_X) + x_ - 3]);

    double line_p1_eig = (d1 + d2) / 2.0;
    double s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static double calc_west_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 2]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1]);

    double line0_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[(y_-1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ +1) * (xsize + EXT_X) + x_ - 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_-1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 3]);

    double line_n1_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ + 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ - 1]);
    d2 = abs((int16_t)indata[(y_+1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 3) * (xsize + EXT_X) + x_ - 2]);

    double line_p1_eig = (d1 + d2) / 2.0;
    double s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static double calc_east_south_eig(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                  uint32_t x_, uint32_t y_)
{
    int32_t d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 2]);
    int32_t d2 = abs((int16_t)indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1]);

    double line0_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[(y_-1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ +1) * (xsize + EXT_X) + x_ + 2]);
    d2 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_+1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 3]);

    double line_n1_eig = (d1 + d2) / 2.0;

    d1 = abs((int16_t)indata[y_ * (xsize + EXT_X) + x_ - 1] - (int16_t)indata[(y_ + 2) * (xsize + EXT_X) + x_ + 1]);
    d2 = abs((int16_t)indata[(y_+1) * (xsize + EXT_X) + x_] - (int16_t)indata[(y_ + 3) * (xsize + EXT_X) + x_ + 2]);

    double line_p1_eig = (d1 + d2) / 2.0;
    double s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int16_t estimate_final_g_for_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                       uint32_t x_, uint32_t y_, double east_eig, double west_eig, double south_eig, double north_eig)
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

    double g_hor_estimate = (g_east_estimate * west_eig + g_west_estimate * east_eig) / (west_eig + east_eig);
    double g_ver_estimate = (g_north_estimate * south_eig + g_south_estimate * north_eig) / (south_eig + north_eig);
    double g_dia_estimate = (g_north_estimate / north_eig + g_south_estimate / south_eig + g_east_estimate / east_eig + g_west_estimate / west_eig) /
                            (1.0 / north_eig + 1.0 / south_eig + 1.0 / east_eig + 1.0 / west_eig);

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

static int16_t estimate_final_rb_for_br(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y,
                                      uint32_t x_, uint32_t y_, double en_eig, double wn_eig, double es_eig, double ws_eig)
{
    double T = 1.9;
    int32_t T_cd = 6;

    int32_t p = indata[y_ * (xsize + EXT_X) + x_];
    int32_t p_en1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
    int32_t p_en2 = indata[(y_ - 2) * (xsize + EXT_X) + x_ + 2];
    int32_t p_en3 = indata[(y_ - 2) * (xsize + EXT_X) + x_ + 3];

    int32_t p_wn1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
    int32_t p_wn2 = indata[(y_ - 2) * (xsize + EXT_X) + x_ - 2];
    int32_t p_wn3 = indata[(y_ - 2) * (xsize + EXT_X) + x_ - 3];

    int32_t p_es1 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
    int32_t p_es2 = indata[(y_ + 2) * (xsize + EXT_X) + x_ + 2];
    int32_t p_es3 = indata[(y_ + 2) * (xsize + EXT_X) + x_ + 3];

    int32_t p_ws1 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
    int32_t p_ws2 = indata[(y_ + 2) * (xsize + EXT_X) + x_ - 2];
    int32_t p_ws3 = indata[(y_ + 2) * (xsize + EXT_X) + x_ - 3];

    int32_t r_en_estimate = p_en1 + (p - p_en2) / 2 + (p_ws1 - 2 * p_en1 + p_en3) / 8;
    int32_t r_wn_estimate = p_wn1 + (p - p_wn2) / 2 + (p_es1 - 2 * p_wn1 + p_wn3) / 8;
    int32_t r_es_estimate = p_es1 + (p - p_es2) / 2 + (p_wn1 - 2 * p_es1 + p_es3) / 8;
    int32_t r_ws_estimate = p_ws1 + (p - p_ws2) / 2 + (p_en1 - 2 * p_ws1 + p_ws3) / 8;

    double r_dia1_estimate = (r_en_estimate * ws_eig + r_ws_estimate * en_eig) / (ws_eig + en_eig);
    double r_dia2_estimate = (r_wn_estimate * es_eig + r_es_estimate * wn_eig) / (es_eig + wn_eig);
    double r_dia_estimate = (r_en_estimate / en_eig + r_wn_estimate / wn_eig + r_es_estimate / es_eig + r_ws_estimate / ws_eig) /
                            (1.0 / en_eig + 1.0 / wn_eig + 1.0 / es_eig + 1.0 / ws_eig);

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

    double hor_estimate = (east_estimate * west_eig + west_estimate * east_eig) / (west_eig + east_eig);
    double ver_estimate = (north_estimate * south_eig + south_estimate * north_eig) / (south_eig + north_eig);
    double dia_estimate = (north_estimate / north_eig + south_estimate / south_eig + east_estimate / east_eig + west_estimate / west_eig) /
                            (1.0 / north_eig + 1.0 / south_eig + 1.0 / east_eig + 1.0 / west_eig);

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

static void demosaic_hw_core(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
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
                double east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double en_eig = calc_east_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double wn_eig = calc_west_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double es_eig = calc_east_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double ws_eig = calc_west_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];

                g_out[row * xsize + col] = estimate_final_g_for_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, east_eig,
                                                                   west_eig, south_eig, north_eig);
                b_out[row * xsize + col] = estimate_final_rb_for_br(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_, 
                    en_eig, wn_eig, es_eig, ws_eig);
            }
            else if (pix_type == PIX_GR)
            {
                double east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

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
                double east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

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
                double east_eig = calc_east_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double west_eig = calc_west_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double north_eig = calc_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double south_eig = calc_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

                double en_eig = calc_east_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double wn_eig = calc_west_north_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double es_eig = calc_east_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                double ws_eig = calc_west_south_eig(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);

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
}

void demosaic_hw::hw_run(statistic_info_t *stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer *input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    if (in.size() > 1)
    {
        fe_module_reg_t *fe_reg = (fe_module_reg_t *)(in[1]->data_ptr);
        memcpy(demosaic_reg, &fe_reg->demosaic_reg, sizeof(demosaic_reg_t));
    }

    demosaic_reg->bypass = bypass;
    if (xmlConfigValid)
    {
    }

    checkparameters(demosaic_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer *output_r = new data_buffer(xsize, ysize, DATA_TYPE_R, BAYER_UNSUPPORT, "demosaic_out0");
    uint16_t *r_ptr = output_r->data_ptr;
    out[0] = output_r;

    data_buffer *output_g = new data_buffer(xsize, ysize, DATA_TYPE_G, BAYER_UNSUPPORT, "demosaic_out1");
    uint16_t *g_ptr = output_g->data_ptr;
    out[1] = output_g;

    data_buffer *output_b = new data_buffer(xsize, ysize, DATA_TYPE_B, BAYER_UNSUPPORT, "demosaic_out2");
    uint16_t *b_ptr = output_b->data_ptr;
    out[2] = output_b;

    const uint32_t EXT_W = 8; // 2;
    const uint32_t EXT_H = 8; // 2;

    std::unique_ptr<uint16_t[]> tmp_data(new uint16_t[(xsize + EXT_W) * (ysize + EXT_H)]);
    uint16_t *tmp = tmp_data.get(); // new uint16_t[(xsize + EXT_W)*(ysize + EXT_H)]; //ext boarder

    for (uint32_t y = EXT_H / 2; y < ysize + EXT_H / 2; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = input_raw->data_ptr[(y - EXT_H / 2) * xsize + x - EXT_W / 2] >> (16 - 14);
        }
    }
    // top
    for (uint32_t y = 0; y < EXT_H / 2; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[(EXT_H - y) * (xsize + EXT_W) + x];
        }
    }
    // bottom
    for (uint32_t y = ysize + EXT_H / 2; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[(2 * ysize + EXT_H - 2 - y) * (xsize + EXT_W) + x];
        }
    }
    // left
    for (uint32_t y = 0; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = 0; x < EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[y * (xsize + EXT_W) + (EXT_W - x)];
        }
    }
    // right
    for (uint32_t y = 0; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = xsize + EXT_W / 2; x < xsize + EXT_W; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[y * (xsize + EXT_W) + (2 * xsize + EXT_W - 2 - x)];
        }
    }

    if (demosaic_reg->bypass == 0)
    {
        demosaic_hw_core(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
    }
    else
    {
        demosaic_hw_core_bilinear(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
    }

    for (uint32_t sz = 0; sz < xsize * ysize; sz++)
    {
        r_ptr[sz] = r_ptr[sz] << (16 - 14);
        g_ptr[sz] = g_ptr[sz] << (16 - 14);
        b_ptr[sz] = b_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void demosaic_hw::hw_init()
{
    log_info("%s init run start\n", name);

    cfgEntry_t config[] = {
        {"bypass", UINT_32, &this->bypass}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

demosaic_hw::~demosaic_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (demosaic_reg != nullptr)
    {
        delete demosaic_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void demosaic_hw::checkparameters(demosaic_reg_t *reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
}
