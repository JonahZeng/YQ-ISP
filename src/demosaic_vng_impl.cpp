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

static int32_t calc_gradient_north(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ - 1) * WIDTH + x_]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 1]) +
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ - 1]);

    int32_t dp1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 1]) +
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ + 1]);

    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_south(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ + 1) * WIDTH + x_]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 2) * WIDTH + x_ - 1]) +
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ - 1]);

    int32_t dp1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 2) * WIDTH + x_ + 1]) +
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ + 1]);

    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_east(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ + 2]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[y_ * WIDTH + x_ + 1]);

    int32_t dn1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 2]) +
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ + 1]);

    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ + 1) * WIDTH + x_ + 2]) +
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ + 1]);

    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_west(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ - 2]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[y_ * WIDTH + x_ - 1]);

    int32_t dn1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 2]) +
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ - 1]);

    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ + 1) * WIDTH + x_ - 2]) +
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ - 1]);

    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_ne_g(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 2]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1)* WIDTH + x_ + 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ - 1) * WIDTH + x_ + 2]);


    return d0 + dn1 + dp1;
}

static int32_t calc_gradient_nw_g(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 2]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1)* WIDTH + x_ - 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ - 1) * WIDTH + x_ - 2]);

    return d0 + dn1 + dp1;
}

static int32_t calc_gradient_sw_g(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ - 2]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1)* WIDTH + x_ - 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 2) * WIDTH + x_ - 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ + 1) * WIDTH + x_ - 2]);

    return d0 + dn1 + dp1;
}

static int32_t calc_gradient_se_g(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ + 2]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1)* WIDTH + x_ + 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 2) * WIDTH + x_ + 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ + 1) * WIDTH + x_ + 2]);

    return d0 + dn1 + dp1;
}

static int32_t calc_gradient_ne_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 2]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1)* WIDTH + x_ + 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ + 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ + 1]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ + 2]);
    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_nw_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 2]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1)* WIDTH + x_ - 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[(y_ - 2) * WIDTH + x_ - 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ - 1]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ - 1) * WIDTH + x_ - 2]);
    return d0 + (dn1 + dp1)/2;
}


static int32_t calc_gradient_sw_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ - 2]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1)* WIDTH + x_ - 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ - 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ - 1]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ - 2]);
    return d0 + (dn1 + dp1)/2;
}

static int32_t calc_gradient_se_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, uint32_t x_, uint32_t y_)
{
    const uint32_t WIDTH = xsize+EXT_X/2;
    int32_t d0 = abs((int32_t)indata[y_ * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ + 2]) + 
        abs((int32_t)indata[(y_ - 1) * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1)* WIDTH + x_ + 1]);

    int32_t dn1 = abs((int32_t)indata[y_ * WIDTH + x_ - 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_]) + 
        abs((int32_t)indata[(y_ + 1) * WIDTH + x_] - (int32_t)indata[(y_ + 2) * WIDTH + x_ + 1]);
    int32_t dp1 = abs((int32_t)indata[(y_ - 1) * WIDTH + x_] - (int32_t)indata[y_ * WIDTH + x_ + 1]) + 
        abs((int32_t)indata[y_ * WIDTH + x_ + 1] - (int32_t)indata[(y_ + 1) * WIDTH + x_ + 2]);
    return d0 + (dn1 + dp1)/2;
}


static void find_max_min(int32_t* arr, size_t len, int32_t* max_v, int32_t* min_v)
{
    *max_v = arr[0];
    *min_v = arr[0];
    for(int32_t i=1; i<len; ++i)
    {
        if(arr[i]> *max_v)
        {
            *max_v = arr[i];
        }
        if(arr[i] < *min_v)
        {
            *min_v = arr[i];
        }
    }
}



void demosaic_hw_core_vng(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
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

            const int32_t WIDTH = xsize + EXT_X;

            if(pix_type == PIX_R)
            {
                int32_t gradient_n = calc_gradient_north(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_s = calc_gradient_south(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_e = calc_gradient_east(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_w = calc_gradient_west(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_ne = calc_gradient_ne_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_nw = calc_gradient_nw_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_se = calc_gradient_se_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_sw = calc_gradient_sw_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t max_v, min_v;
                int32_t arr[8] = {gradient_n, gradient_s, gradient_e, gradient_w, 
                    gradient_ne, gradient_nw, gradient_se, gradient_sw};
                find_max_min(arr, 8, &max_v, &min_v);
                int32_t thr = static_cast<int32_t>(1.5*min_v + 0.5*(min_v + max_v));

                int32_t small_grad_cnt = 0;
                for(int32_t i=0; i<8; ++i)
                {
                    if(arr[i] < thr)
                    {
                        small_grad_cnt += 1;
                    }
                }
                int32_t g_sum = 0;
                int32_t b_sum = 0;
                int32_t r_sum = 0;
                if(gradient_n < thr)
                {
                    r_sum += (indata[y_*WIDTH+x_] + indata[(y_-2)*WIDTH+x_])/2;
                    g_sum += indata[(y_-1)*WIDTH + x_];
                    b_sum += (indata[(y_-1)*WIDTH+x_-1] + indata[(y_-1)*WIDTH+x_+1])/2;
                }
                if(gradient_s < thr)
                {
                    r_sum += (indata[y_*WIDTH+x_] + indata[(y_+2)*WIDTH+x_])/2;
                    g_sum += indata[(y_+1)*WIDTH + x_];
                    b_sum += (indata[(y_+1)*WIDTH+x_-1] + indata[(y_+1)*WIDTH+x_+1])/2;
                }
                if(gradient_e < thr)
                {
                    r_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_+2])/2;
                    g_sum += indata[y_*WIDTH + x_+1];
                    b_sum += (indata[(y_-1)*WIDTH+x_+1] + indata[(y_+1)*WIDTH+x_+1])/2;
                }
                if(gradient_w < thr)
                {
                    r_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_-2])/2;
                    g_sum += indata[y_*WIDTH + x_-1];
                    b_sum += (indata[(y_-1)*WIDTH+x_-1] + indata[(y_+1)*WIDTH+x_-1])/2;
                }
                if(gradient_ne < thr)
                {
                    g_sum += (indata[(y_ - 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ + 1] + indata[(y_ - 1) * WIDTH + x_ + 2] + indata[(y_ - 2) * WIDTH + x_ + 1]) / 4;
                    r_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ + 2] + indata[(y_ - 2) * WIDTH + x_] + indata[(y_ - 2) * WIDTH + x_ + 2]) / 4;
                    b_sum += indata[(y_ - 1) * WIDTH + x_ + 1];
                }
                if(gradient_nw < thr)
                {
                    g_sum += (indata[(y_ - 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ - 1] + indata[(y_ - 1) * WIDTH + x_ - 2] + indata[(y_ - 2) * WIDTH + x_ - 1]) / 4;
                    r_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ - 2] + indata[(y_ - 2) * WIDTH + x_] + indata[(y_ - 2) * WIDTH + x_ - 2]) / 4;
                    b_sum += indata[(y_ - 1) * WIDTH + x_ - 1];
                }
                if(gradient_sw < thr)
                {
                    g_sum += (indata[(y_ + 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ - 1] + indata[(y_ + 1) * WIDTH + x_ - 2] + indata[(y_ + 2) * WIDTH + x_ - 1]) / 4;
                    r_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ - 2] + indata[(y_ + 2) * WIDTH + x_] + indata[(y_ + 2) * WIDTH + x_ - 2]) / 4;
                    b_sum += indata[(y_ + 1) * WIDTH + x_ - 1];
                }
                if(gradient_se < thr)
                {
                    g_sum += (indata[(y_ + 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ + 1] + indata[(y_ + 1) * WIDTH + x_ + 2] + indata[(y_ + 2) * WIDTH + x_ + 1]) / 4;
                    r_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ + 2] + indata[(y_ + 2) * WIDTH + x_] + indata[(y_ + 2) * WIDTH + x_ + 2]) / 4;
                    b_sum += indata[(y_ + 1) * WIDTH + x_ + 1];
                }

                r_out[row*xsize+col] = indata[y_*WIDTH+x_];
                int32_t b_o = (int32_t)indata[y_*WIDTH+x_] + (b_sum - r_sum)/small_grad_cnt;
                b_out[row*xsize+col] = CLIP3(b_o, 0, 16383);
                int32_t g_o = (int32_t)indata[y_*WIDTH+x_] + (g_sum - r_sum)/small_grad_cnt;
                g_out[row*xsize+col] = CLIP3(g_o, 0, 16383);
            }
            else if(pix_type == PIX_GR)
            {
                int32_t gradient_n = calc_gradient_north(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_s = calc_gradient_south(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_e = calc_gradient_east(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_w = calc_gradient_west(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_ne = calc_gradient_ne_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_nw = calc_gradient_nw_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_se = calc_gradient_se_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_sw = calc_gradient_sw_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t max_v, min_v;
                int32_t arr[8] = {gradient_n, gradient_s, gradient_e, gradient_w, 
                    gradient_ne, gradient_nw, gradient_se, gradient_sw};
                find_max_min(arr, 8, &max_v, &min_v);
                int32_t thr = static_cast<int32_t>(1.5*min_v + 0.5*(min_v + max_v));

                //esmate b up down, r left right
                int32_t small_grad_cnt = 0;
                for(int32_t i=0; i<8; ++i)
                {
                    if(arr[i] < thr)
                    {
                        small_grad_cnt += 1;
                    }
                }
                int32_t g_sum = 0;
                int32_t b_sum = 0;
                int32_t r_sum = 0;
                if(gradient_n < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[(y_-2)*WIDTH+x_])/2;
                    b_sum += indata[(y_-1)*WIDTH + x_];
                    r_sum += (indata[y_*WIDTH+x_-1] + indata[y_*WIDTH+x_+1] + indata[(y_-2)*WIDTH+x_-1] + indata[(y_-2)*WIDTH+x_+1])/4;
                }
                if(gradient_s < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[(y_+2)*WIDTH+x_])/2;
                    b_sum += indata[(y_+1)*WIDTH + x_];
                    r_sum += (indata[y_*WIDTH+x_-1] + indata[y_*WIDTH+x_+1] + indata[(y_+2)*WIDTH+x_-1] + indata[(y_+2)*WIDTH+x_+1])/4;
                }
                if(gradient_e < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_+2])/2;
                    r_sum += indata[y_*WIDTH + x_+1];
                    b_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_+2] + indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_+2])/4;
                }
                if(gradient_w < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_-2])/2;
                    r_sum += indata[y_*WIDTH + x_-1];
                    b_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_-2] + indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_-2])/4;
                }
                if(gradient_ne < thr)
                {
                    g_sum += indata[(y_-1)*WIDTH+x_+1];
                    b_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_+2])/2;
                    r_sum += (indata[y_*WIDTH+x_+1] + indata[(y_-2)*WIDTH+x_+1])/2;
                }
                if(gradient_nw < thr)
                {
                    g_sum += indata[(y_-1)*WIDTH+x_-1];
                    b_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_-2])/2;
                    r_sum += (indata[y_*WIDTH+x_-1] + indata[(y_-2)*WIDTH+x_-1])/2;
                }
                if(gradient_sw < thr)
                {
                    g_sum += indata[(y_+1)*WIDTH+x_-1];
                    b_sum += (indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_-2])/2;
                    r_sum += (indata[y_*WIDTH+x_-1] + indata[(y_+2)*WIDTH+x_-1])/2;
                }
                if(gradient_se < thr)
                {
                    g_sum += indata[(y_+1)*WIDTH+x_+1];
                    b_sum += (indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_+2])/2;
                    r_sum += (indata[y_*WIDTH+x_+1] + indata[(y_+2)*WIDTH+x_+1])/2;
                }

                g_out[row*xsize+col] = indata[y_*WIDTH+x_];
                int32_t b_o = (int32_t)indata[y_*WIDTH+x_] + (b_sum - g_sum)/small_grad_cnt;
                b_out[row*xsize+col] = CLIP3(b_o, 0, 16383);
                int32_t r_o = (int32_t)indata[y_*WIDTH+x_] + (r_sum - g_sum)/small_grad_cnt;
                r_out[row*xsize+col] = CLIP3(r_o, 0, 16383);
            }
            else if(pix_type == PIX_GB)
            {
                int32_t gradient_n = calc_gradient_north(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_s = calc_gradient_south(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_e = calc_gradient_east(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_w = calc_gradient_west(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_ne = calc_gradient_ne_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_nw = calc_gradient_nw_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_se = calc_gradient_se_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_sw = calc_gradient_sw_g(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t max_v, min_v;
                int32_t arr[8] = {gradient_n, gradient_s, gradient_e, gradient_w, 
                    gradient_ne, gradient_nw, gradient_se, gradient_sw};
                find_max_min(arr, 8, &max_v, &min_v);
                int32_t thr = static_cast<int32_t>(1.5*min_v + 0.5*(min_v + max_v));
                //esmate r up down, b left right
                int32_t small_grad_cnt = 0;
                for(int32_t i=0; i<8; ++i)
                {
                    if(arr[i] < thr)
                    {
                        small_grad_cnt += 1;
                    }
                }
                int32_t g_sum = 0;
                int32_t b_sum = 0;
                int32_t r_sum = 0;
                if(gradient_n < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[(y_-2)*WIDTH+x_])/2;
                    r_sum += indata[(y_-1)*WIDTH + x_];
                    b_sum += (indata[y_*WIDTH+x_-1] + indata[y_*WIDTH+x_+1] + indata[(y_-2)*WIDTH+x_-1] + indata[(y_-2)*WIDTH+x_+1])/4;
                }
                if(gradient_s < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[(y_+2)*WIDTH+x_])/2;
                    r_sum += indata[(y_+1)*WIDTH + x_];
                    b_sum += (indata[y_*WIDTH+x_-1] + indata[y_*WIDTH+x_+1] + indata[(y_+2)*WIDTH+x_-1] + indata[(y_+2)*WIDTH+x_+1])/4;
                }
                if(gradient_e < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_+2])/2;
                    b_sum += indata[y_*WIDTH + x_+1];
                    r_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_+2] + indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_+2])/4;
                }
                if(gradient_w < thr)
                {
                    g_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_-2])/2;
                    b_sum += indata[y_*WIDTH + x_-1];
                    r_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_-2] + indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_-2])/4;
                }
                if(gradient_ne < thr)
                {
                    g_sum += indata[(y_-1)*WIDTH+x_+1];
                    r_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_+2])/2;
                    b_sum += (indata[y_*WIDTH+x_+1] + indata[(y_-2)*WIDTH+x_+1])/2;
                }
                if(gradient_nw < thr)
                {
                    g_sum += indata[(y_-1)*WIDTH+x_-1];
                    r_sum += (indata[(y_-1)*WIDTH+x_] + indata[(y_-1)*WIDTH+x_-2])/2;
                    b_sum += (indata[y_*WIDTH+x_-1] + indata[(y_-2)*WIDTH+x_-1])/2;
                }
                if(gradient_sw < thr)
                {
                    g_sum += indata[(y_+1)*WIDTH+x_-1];
                    r_sum += (indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_-2])/2;
                    b_sum += (indata[y_*WIDTH+x_-1] + indata[(y_+2)*WIDTH+x_-1])/2;
                }
                if(gradient_se < thr)
                {
                    g_sum += indata[(y_+1)*WIDTH+x_+1];
                    r_sum += (indata[(y_+1)*WIDTH+x_] + indata[(y_+1)*WIDTH+x_+2])/2;
                    b_sum += (indata[y_*WIDTH+x_+1] + indata[(y_+2)*WIDTH+x_+1])/2;
                }

                g_out[row*xsize+col] = indata[y_*WIDTH+x_];
                int32_t b_o = (int32_t)indata[y_*WIDTH+x_] + (b_sum - g_sum)/small_grad_cnt;
                b_out[row*xsize+col] = CLIP3(b_o, 0, 16383);
                int32_t r_o = (int32_t)indata[y_*WIDTH+x_] + (r_sum - g_sum)/small_grad_cnt;
                r_out[row*xsize+col] = CLIP3(r_o, 0, 16383);
            }
            else if(pix_type == PIX_B)
            {
                int32_t gradient_n = calc_gradient_north(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_s = calc_gradient_south(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_e = calc_gradient_east(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_w = calc_gradient_west(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_ne = calc_gradient_ne_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_nw = calc_gradient_nw_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_se = calc_gradient_se_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t gradient_sw = calc_gradient_sw_rb(indata, xsize, ysize, EXT_X, EXT_Y, x_, y_);
                int32_t max_v, min_v;
                int32_t arr[8] = {gradient_n, gradient_s, gradient_e, gradient_w, 
                    gradient_ne, gradient_nw, gradient_se, gradient_sw};
                find_max_min(arr, 8, &max_v, &min_v);
                int32_t thr = static_cast<int32_t>(1.5*min_v + 0.5*(min_v + max_v));

                int32_t small_grad_cnt = 0;
                for(int32_t i=0; i<8; ++i)
                {
                    if(arr[i] < thr)
                    {
                        small_grad_cnt += 1;
                    }
                }
                int32_t g_sum = 0;
                int32_t b_sum = 0;
                int32_t r_sum = 0;
                if(gradient_n < thr)
                {
                    b_sum += (indata[y_*WIDTH+x_] + indata[(y_-2)*WIDTH+x_])/2;
                    g_sum += indata[(y_-1)*WIDTH + x_];
                    r_sum += (indata[(y_-1)*WIDTH+x_-1] + indata[(y_-1)*WIDTH+x_+1])/2;
                }
                if(gradient_s < thr)
                {
                    b_sum += (indata[y_*WIDTH+x_] + indata[(y_+2)*WIDTH+x_])/2;
                    g_sum += indata[(y_+1)*WIDTH + x_];
                    r_sum += (indata[(y_+1)*WIDTH+x_-1] + indata[(y_+1)*WIDTH+x_+1])/2;
                }
                if(gradient_e < thr)
                {
                    b_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_+2])/2;
                    g_sum += indata[y_*WIDTH + x_+1];
                    r_sum += (indata[(y_-1)*WIDTH+x_+1] + indata[(y_+1)*WIDTH+x_+1])/2;
                }
                if(gradient_w < thr)
                {
                    b_sum += (indata[y_*WIDTH+x_] + indata[y_*WIDTH+x_-2])/2;
                    g_sum += indata[y_*WIDTH + x_-1];
                    r_sum += (indata[(y_-1)*WIDTH+x_-1] + indata[(y_+1)*WIDTH+x_-1])/2;
                }
                if(gradient_ne < thr)
                {
                    g_sum += (indata[(y_ - 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ + 1] + indata[(y_ - 1) * WIDTH + x_ + 2] + indata[(y_ - 2) * WIDTH + x_ + 1]) / 4;
                    b_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ + 2] + indata[(y_ - 2) * WIDTH + x_] + indata[(y_ - 2) * WIDTH + x_ + 2]) / 4;
                    r_sum += indata[(y_ - 1) * WIDTH + x_ + 1];
                }
                if(gradient_nw < thr)
                {
                    g_sum += (indata[(y_ - 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ - 1] + indata[(y_ - 1) * WIDTH + x_ - 2] + indata[(y_ - 2) * WIDTH + x_ - 1]) / 4;
                    b_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ - 2] + indata[(y_ - 2) * WIDTH + x_] + indata[(y_ - 2) * WIDTH + x_ - 2]) / 4;
                    r_sum += indata[(y_ - 1) * WIDTH + x_ - 1];
                }
                if(gradient_sw < thr)
                {
                    g_sum += (indata[(y_ + 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ - 1] + indata[(y_ + 1) * WIDTH + x_ - 2] + indata[(y_ + 2) * WIDTH + x_ - 1]) / 4;
                    b_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ - 2] + indata[(y_ + 2) * WIDTH + x_] + indata[(y_ + 2) * WIDTH + x_ - 2]) / 4;
                    r_sum += indata[(y_ + 1) * WIDTH + x_ - 1];
                }
                if(gradient_se < thr)
                {
                    g_sum += (indata[(y_ + 1) * WIDTH + x_] + indata[y_ * WIDTH + x_ + 1] + indata[(y_ + 1) * WIDTH + x_ + 2] + indata[(y_ + 2) * WIDTH + x_ + 1]) / 4;
                    b_sum += (indata[y_ * WIDTH + x_] + indata[y_ * WIDTH + x_ + 2] + indata[(y_ + 2) * WIDTH + x_] + indata[(y_ + 2) * WIDTH + x_ + 2]) / 4;
                    r_sum += indata[(y_ + 1) * WIDTH + x_ + 1];
                }

                b_out[row*xsize+col] = indata[y_*WIDTH+x_];
                int32_t r_o = (int32_t)indata[y_*WIDTH+x_] + (r_sum - b_sum)/small_grad_cnt;
                r_out[row*xsize+col] = CLIP3(r_o, 0, 16383);
                int32_t g_o = (int32_t)indata[y_*WIDTH+x_] + (g_sum - b_sum)/small_grad_cnt;
                g_out[row*xsize+col] = CLIP3(g_o, 0, 16383);
            }
        }
    }
}