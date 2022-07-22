#include "pipe_register.h"

#define FLIP(x, max_v) ((x) < 0 ? (-(x)) : ((x) > (max_v) ? 2 * (max_v) - (x) : (x)))
#define CLIP3(x, min_v, max_v) ((x) < (min_v) ? (min_v) : ((x) > (max_v) ? (max_v) : (x)))
#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MIN2(a, b) ((a) > (b) ? (b) : (a))

#define REPEAT4(x, max_v) ((x) < 0 ? (4 + (x)) : ((x) > (max_v) ? ((x)-4) : (x)))

rgbir_remosaic_hw::rgbir_remosaic_hw(uint32_t inpins, uint32_t outpins, const char* inst_name)
    :hw_base(inpins, outpins, inst_name), bypass(0), rgbir_remosaic_reg(new rgbir_remosaic_reg_t)
{

}

static pixel_bayer_type get_pixel_bayer_type_ir(uint32_t y, uint32_t x, bayer_type_t by)
{
    uint32_t pos = ((y % 4) * 4) + (x % 4);

    pixel_bayer_type type[16];
    if (by == RGGIR)
    {
        type[0] = PIX_R; type[1] = PIX_GR; type[2] = PIX_B; type[3] = PIX_GB;
        type[4] = PIX_GB; type[5] = PIX_Y; type[6] = PIX_GR; type[7] = PIX_Y;
        type[8] = PIX_B; type[9] = PIX_GB; type[10] = PIX_R; type[11] = PIX_GR;
        type[12] = PIX_GR; type[13] = PIX_Y; type[14] = PIX_GB; type[15] = PIX_Y;
    }
    else if (by == BGGIR)
    {
        type[0] = PIX_B; type[1] = PIX_GB; type[2] = PIX_R; type[3] = PIX_GR;
        type[4] = PIX_GR; type[5] = PIX_Y; type[6] = PIX_GB; type[7] = PIX_Y;
        type[8] = PIX_R; type[9] = PIX_GR; type[10] = PIX_B; type[11] = PIX_GB;
        type[12] = PIX_GB; type[13] = PIX_Y; type[14] = PIX_GR; type[15] = PIX_Y;
    }
    else if (by == GRIRG)
    {
        type[0] = PIX_GR; type[1] = PIX_R; type[2] = PIX_GB; type[3] = PIX_B;
        type[4] = PIX_Y; type[5] = PIX_GB; type[6] = PIX_Y; type[7] = PIX_GR;
        type[8] = PIX_GB; type[9] = PIX_B; type[10] = PIX_GR; type[11] = PIX_R;
        type[12] = PIX_Y; type[13] = PIX_GR; type[14] = PIX_Y; type[15] = PIX_GB;
    }
    else if(by == GBIRG)
    {
        type[0] = PIX_GB; type[1] = PIX_B; type[2] = PIX_GR; type[3] = PIX_R;
        type[4] = PIX_Y; type[5] = PIX_GR; type[6] = PIX_Y; type[7] = PIX_GB;
        type[8] = PIX_GR; type[9] = PIX_R; type[10] = PIX_GB; type[11] = PIX_B;
        type[12] = PIX_Y; type[13] = PIX_GB; type[14] = PIX_Y; type[15] = PIX_GR;
    }
    else if(by == IRGGR)
    {
        type[0] = PIX_Y; type[1] = PIX_GB; type[2] = PIX_Y; type[3] = PIX_GR;
        type[4] = PIX_GR; type[5] = PIX_R; type[6] = PIX_GB; type[7] = PIX_B;
        type[8] = PIX_Y; type[9] = PIX_GR; type[10] = PIX_Y; type[11] = PIX_GB;
        type[12] = PIX_GB; type[13] = PIX_B; type[14] = PIX_GR; type[15] = PIX_R;
    }
    else if(by == GIRBG)
    {
        type[0] = PIX_GR; type[1] = PIX_Y; type[2] = PIX_GB; type[3] = PIX_Y;
        type[4] = PIX_B; type[5] = PIX_GB; type[6] = PIX_R; type[7] = PIX_GR;
        type[8] = PIX_GB; type[9] = PIX_Y; type[10] = PIX_GR; type[11] = PIX_Y;
        type[12] = PIX_R; type[13] = PIX_GR; type[14] = PIX_B; type[15] = PIX_GB;
    }
    else if(by == IRGGB)
    {
        type[0] = PIX_Y; type[1] = PIX_GR; type[2] = PIX_Y; type[3] = PIX_GB;
        type[4] = PIX_GB; type[5] = PIX_B; type[6] = PIX_GR; type[7] = PIX_R;
        type[8] = PIX_Y; type[9] = PIX_GB; type[10] = PIX_Y; type[11] = PIX_GR;
        type[12] = PIX_GR; type[13] = PIX_R; type[14] = PIX_GB; type[15] = PIX_B;
    }
    else if(by == GIRRG)
    {
        type[0] = PIX_GB; type[1] = PIX_Y; type[2] = PIX_GR; type[3] = PIX_Y;
        type[4] = PIX_R; type[5] = PIX_GR; type[6] = PIX_B; type[7] = PIX_GB;
        type[8] = PIX_GR; type[9] = PIX_Y; type[10] = PIX_GB; type[11] = PIX_Y;
        type[12] = PIX_B; type[13] = PIX_GB; type[14] = PIX_R; type[15] = PIX_GR;
    }

    return type[pos];
}

static int32_t calc_east_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ + 3, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)]);

    int32_t line0_eig = d1 + d2;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)]);

    int32_t line_n1_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)]);

    int32_t line_p1_eig = d1;

    int32_t e_eig = line0_eig + line_n1_eig + line_p1_eig;

    return e_eig;
}

static int32_t calc_west_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ - 3, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    
    int32_t line0_eig = d1 + d2;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)]);
    
    int32_t line_n1_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)]);

    int32_t line_p1_eig = d1;

    int32_t w_eig = line0_eig + line_n1_eig + line_p1_eig;

    return w_eig;
}

static int32_t calc_north_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ - 3, MAX_ROW) * xsize + x_]);
    int32_t line0_eig = d1 + d2;

    d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);

    int32_t line_n1_eig = d1;

    d1 = abs((int16_t)indata[y_  * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);

    int32_t line_p1_eig = d1;

    int32_t n_eig = line0_eig + line_n1_eig + line_p1_eig;

    return n_eig;
}

static int32_t calc_south_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[REPEAT4(y_ + 3, MAX_ROW) * xsize + x_]);

    int32_t line0_eig = d1 + d2;

    d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);

    int32_t line_n1_eig = d1;

    d1 = abs((int16_t)indata[y_  * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);

    int32_t line_p1_eig = d1;

    int32_t s_eig = line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t calc_east_north_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[y_* xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_]);

    int32_t d3 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_]);
    int32_t d4 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)]);

    int32_t  en_eig = d1 + d2 + d3 + d4;

    return en_eig;
}

static int32_t calc_west_north_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[y_* xsize + REPEAT4(x_ + 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_]);

    int32_t d3 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_]);
    int32_t d4 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)]);

    int32_t  wn_eig = d1 + d2 + d3 + d4;

    return wn_eig;
}

static int32_t calc_west_south_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)]);

    int32_t d3 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    int32_t d4 = abs((int16_t)indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_]);

    int32_t  ws_eig = d1 + d2 + d3 + d4;

    return ws_eig;
}

static int32_t calc_east_south_eig_rb(uint16_t *indata, uint32_t xsize, uint32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t d1 = abs((int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + x_]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ + 1, MAX_COL)]);

    int32_t d3 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_] - 
        (int16_t)indata[y_ * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d4 = abs((int16_t)indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + x_]);

    int32_t  es_eig = d1 + d2 + d3 + d4;

    return es_eig;
}

static int32_t calc_east_eig_g(uint16_t *indata, int32_t xsize, int32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;

    int32_t d1 = abs((int16_t)indata[y_ * xsize + x_] - (int16_t)indata[y_ * xsize + REPEAT4(x_ + 2, MAX_COL)]);

    int32_t line0_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 3, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t line_n1_eig = ci_e;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 3, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    ci_e = (d1 + d2) / 2;

    int32_t line_p1_eig = ci_e;
    int32_t e_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return e_eig;
}

static int32_t calc_west_eig_g(uint16_t *indata, int32_t xsize, int32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;

    int32_t d1 = abs((int16_t)indata[y_ * xsize + x_] - (int16_t)indata[y_ * xsize + REPEAT4(x_ - 2, MAX_COL)]);

    int32_t line0_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 3, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t line_n1_eig = ci_e;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 3, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    ci_e = (d1 + d2) / 2;

    int32_t line_p1_eig = ci_e;
    int32_t w_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return w_eig;
}

static int32_t calc_north_eig_g(uint16_t *indata, int32_t xsize, int32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;

    int32_t d1 = abs((int16_t)indata[y_ * xsize + x_] - (int16_t)indata[REPEAT4(y_ - 2, MAX_ROW)* xsize + x_]);

    int32_t line0_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 3, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t line_n1_eig = ci_e;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 3, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    ci_e = (d1 + d2) / 2;

    int32_t line_p1_eig = ci_e;
    int32_t n_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return n_eig;
}

static int32_t calc_south_eig_g(uint16_t *indata, int32_t xsize, int32_t ysize, int32_t x_, int32_t y_)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;

    int32_t d1 = abs((int16_t)indata[y_ * xsize + x_] - (int16_t)indata[REPEAT4(y_ + 2, MAX_ROW)* xsize + x_]);

    int32_t line0_eig = d1;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t d2 = abs((int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 3, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)]);
    int32_t ci_e = (d1 + d2) / 2;

    int32_t line_n1_eig = ci_e;

    d1 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    d2 = abs((int16_t)indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)] - 
        (int16_t)indata[REPEAT4(y_ + 3, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)]);
    ci_e = (d1 + d2) / 2;

    int32_t line_p1_eig = ci_e;
    int32_t s_eig = 2 * line0_eig + line_n1_eig + line_p1_eig;

    return s_eig;
}

static int32_t estimate_dia1_cd(uint16_t *src, int32_t x_, int32_t y_, int32_t *filter, int32_t xsize, int32_t ysize)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)src[REPEAT4(y_ + 2 - i, MAX_ROW) * (int32_t)xsize + REPEAT4(x_ - 2 + i, MAX_COL)] * filter[i];
    }
    return ret;
}

static int32_t estimate_dia2_cd(uint16_t *src, int32_t x_, int32_t y_, int32_t *filter, int32_t xsize, int32_t ysize)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    int32_t ret = 0;
    for (int32_t i = 0; i < 5; i++)
    {
        ret += (int32_t)src[REPEAT4(y_ - 2 + i, MAX_ROW) * (int32_t)xsize + REPEAT4(x_ - 2 + i, MAX_COL)] * filter[i];
    }
    return ret;
}

static uint16_t estimate_final_ir_for_rb(uint16_t *indata, int32_t xsize, int32_t ysize, 
                                       int32_t x_, int32_t y_, int32_t en_eig, int32_t wn_eig, int32_t es_eig, int32_t ws_eig)
{
    double T = 1.9;
    int32_t T_cd = 6;

    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;

    int32_t p = indata[y_ * xsize + x_];
    int32_t p_en1 = indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)];
    int32_t p_en2 = indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)];
    int32_t p_en3 = indata[REPEAT4(y_ - 3, MAX_ROW) * xsize + REPEAT4(x_ + 3, MAX_COL)];
    int32_t p_wn1 = indata[REPEAT4(y_ - 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)];
    int32_t p_wn2 = indata[REPEAT4(y_ - 2, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)];
    int32_t p_wn3 = indata[REPEAT4(y_ - 3, MAX_ROW) * xsize + REPEAT4(x_ - 3, MAX_COL)];
    int32_t p_es1 = indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ + 1, MAX_COL)];
    int32_t p_es2 = indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ + 2, MAX_COL)];
    int32_t p_es3 = indata[REPEAT4(y_ + 3, MAX_ROW) * xsize + REPEAT4(x_ + 3, MAX_COL)];
    int32_t p_ws1 = indata[REPEAT4(y_ + 1, MAX_ROW) * xsize + REPEAT4(x_ - 1, MAX_COL)];
    int32_t p_ws2 = indata[REPEAT4(y_ + 2, MAX_ROW) * xsize + REPEAT4(x_ - 2, MAX_COL)];
    int32_t p_ws3 = indata[REPEAT4(y_ + 3, MAX_ROW) * xsize + REPEAT4(x_ - 3, MAX_COL)];
    int32_t ir_en_estimate = p_en1 + (p - p_en2) / 2 + (p_ws1 - 2 * p_en1 + p_en3) / 8;
    int32_t ir_wn_estimate = p_wn1 + (p - p_wn2) / 2 + (p_wn3 - 2 * p_wn1 + p_es1) / 8;
    int32_t ir_es_estimate = p_es1 + (p - p_es2) / 2 + (p_es3 - 2 * p_es1 + p_wn1) / 8;
    int32_t ir_ws_estimate = p_ws1 + (p - p_ws2) / 2 + (p_en1 - 2 * p_ws1 + p_ws3) / 8;

    double ir_d1_estimate = 0.0;
    if(en_eig + ws_eig == 0)
    {
        ir_d1_estimate = (ir_en_estimate + ir_ws_estimate + 1) / 2;
    }
    else{
        ir_d1_estimate = (ir_en_estimate * ws_eig + ir_ws_estimate * en_eig) / (en_eig + ws_eig);
    }

    double ir_d2_estimate = 0.0;
    if(wn_eig + es_eig == 0)
    {
        ir_d2_estimate = (ir_wn_estimate + ir_es_estimate + 1) / 2;
    }
    else{
        ir_d2_estimate = (ir_wn_estimate * es_eig + ir_es_estimate * wn_eig) / (es_eig + wn_eig);
    }

    double ir_com_estimate = 0.0;
    if(ws_eig + en_eig + es_eig + wn_eig == 0)
    {
        ir_com_estimate = (ir_d1_estimate + ir_d2_estimate + 1) / 2;
    }
    else
    {
        ir_com_estimate = (ir_d1_estimate * (es_eig + wn_eig) + ir_d2_estimate * (en_eig + ws_eig)) / (ws_eig + en_eig + es_eig + wn_eig);
    }

    ir_d1_estimate = CLIP3(ir_d1_estimate, 0.0, 16383.0);
    ir_d2_estimate = CLIP3(ir_d2_estimate, 0.0, 16383.0);
    ir_com_estimate = CLIP3(ir_com_estimate, 0.0, 16383.0);

    double d1_eig = en_eig + ws_eig;
    double d2_eig = wn_eig + es_eig;
    double E = 0.0;
    double min_eig = MIN2(d1_eig, d2_eig);
    if(abs(min_eig - 0.0) < 0.000001)
    {
        E = T + 1.0;
    }
    else
    {
        E = MAX2(d1_eig, d2_eig) / MIN2(d1_eig, d2_eig);
    }

    uint16_t out = 0;

    if (E < T)
    {
        int32_t filter[5] = {1, -3, 4, -3, 1};
        int32_t cd_d1_0 = estimate_dia1_cd(indata, x_, y_, filter, xsize, ysize);
        int32_t cd_d1_n2 = estimate_dia1_cd(indata, x_ - 2, y_ + 2, filter, xsize, ysize);
        int32_t cd_d1_p2 = estimate_dia1_cd(indata, x_ + 2, y_ - 2, filter, xsize, ysize);

        int32_t cd_d2_0 = estimate_dia2_cd(indata, x_, y_, filter, xsize, ysize);
        int32_t cd_d2_n2 = estimate_dia2_cd(indata, x_ - 2, y_ - 2, filter, xsize, ysize);
        int32_t cd_d2_p2 = estimate_dia2_cd(indata, x_ + 2, y_ + 2, filter, xsize, ysize);

        int32_t omg_cd_d1 = abs(cd_d1_n2 - cd_d1_0) + abs(cd_d1_p2 - cd_d1_0);
        int32_t omg_cd_d2 = abs(cd_d2_n2 - cd_d2_0) + abs(cd_d2_p2 - cd_d2_0);

        int32_t omg_diff = abs(omg_cd_d1 - omg_cd_d2);
        if (omg_diff < T_cd)
        {
            out = (uint16_t)ir_com_estimate;
        }
        else if (omg_cd_d1 < omg_cd_d2)
        {
            out = (uint16_t)ir_d1_estimate;
        }
        else
        {
            out = (uint16_t)ir_d2_estimate;
        }
    }
    else
    {
        if (d1_eig > d2_eig)
        {
            out = (uint16_t)ir_d2_estimate;
        }
        else
        {
            out = (uint16_t)ir_d1_estimate;
        }
    }
    return out;
}

static void estimate_ir_up_down_for_G(uint16_t *indata, uint16_t *ir_out, int32_t row, int32_t col,
                                        int32_t ysize, int32_t xsize, int32_t* ir_up, int32_t* ir_down)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    if(row >= 1)
    {
        *ir_up = ir_out[(row - 1) * xsize + col];
    }
    else
    {
        int32_t ir_wn = indata[REPEAT4(row - 2, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
        int32_t ir_en = indata[REPEAT4(row - 2, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
        int32_t ir_ws = indata[row * xsize + REPEAT4(col - 1, MAX_COL)];
        int32_t ir_es = indata[row * xsize + REPEAT4(col + 1, MAX_COL)];
        int32_t grad_d1 = abs(ir_en - ir_ws);
        int32_t grad_d2 = abs(ir_wn - ir_es);
        int32_t ir_d1 = (ir_en + ir_ws) / 2;
        int32_t ir_d2 = (ir_wn + ir_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *ir_up = ir_wn;
        }
        else
        {
            *ir_up = (ir_d1 * grad_d2 + ir_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t ir_wn = indata[row * xsize + REPEAT4(col - 1, MAX_COL)];
        int32_t ir_en = indata[row * xsize + REPEAT4(col + 1, MAX_COL)];
        int32_t ir_ws = indata[REPEAT4(row + 2, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
        int32_t ir_es = indata[REPEAT4(row + 2, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
        int32_t grad_d1 = abs(ir_en - ir_ws);
        int32_t grad_d2 = abs(ir_wn - ir_es);
        int32_t ir_d1 = (ir_en + ir_ws) / 2;
        int32_t ir_d2 = (ir_wn + ir_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *ir_down = ir_wn;
        }
        else
        {
            *ir_down = (ir_d1 * grad_d2 + ir_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}

static void estimate_ir_left_right_for_G(uint16_t *indata, uint16_t *ir_out, int32_t row, int32_t col,
                                        int32_t ysize, int32_t xsize, int32_t* ir_left, int32_t* ir_right)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    if(col >= 1)
    {
        *ir_left = ir_out[row * xsize + col - 1];
    }
    else
    {
        int32_t ir_wn = indata[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 2, MAX_COL)];
        int32_t ir_en = indata[REPEAT4(row - 1, MAX_ROW) * xsize + col];
        int32_t ir_ws = indata[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 2, MAX_COL)];
        int32_t ir_es = indata[REPEAT4(row + 1, MAX_ROW) * xsize + col];
        int32_t grad_d1 = abs(ir_en - ir_ws);
        int32_t grad_d2 = abs(ir_wn - ir_es);
        int32_t ir_d1 = (ir_en + ir_ws) / 2;
        int32_t ir_d2 = (ir_wn + ir_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *ir_left = ir_wn;
        }
        else
        {
            *ir_left = (ir_d1 * grad_d2 + ir_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }

    {
        int32_t ir_wn = indata[REPEAT4(row - 1, MAX_ROW) * xsize + col];
        int32_t ir_en = indata[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 2, MAX_COL)];
        int32_t ir_ws = indata[REPEAT4(row + 1, MAX_ROW) * xsize + col];
        int32_t ir_es = indata[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 2, MAX_COL)];
        int32_t grad_d1 = abs(ir_en - ir_ws);
        int32_t grad_d2 = abs(ir_wn - ir_es);
        int32_t ir_d1 = (ir_en + ir_ws) / 2;
        int32_t ir_d2 = (ir_wn + ir_es) / 2;
        if(grad_d1 + grad_d2 == 0)
        {
            *ir_right = ir_wn;
        }
        else
        {
            *ir_right = (ir_d1 * grad_d2 + ir_d2 * grad_d1)/(grad_d1 + grad_d2);
        }
    }
}

static uint16_t estimate_final_ir_for_g(uint16_t *indata, uint32_t xsize, uint32_t ysize,
            uint32_t x_, uint32_t y_, double east_eig, double west_eig, double south_eig, double north_eig,
            int32_t p_up, int32_t p_down, int32_t p_left, int32_t p_right)
{
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    double T = 1.3;

    int32_t p = indata[y_ * xsize + x_];
    int32_t p_right2 = indata[y_ * xsize + x_ + 2];
    int32_t p_left2 = indata[y_ * xsize + x_ - 2];
    int32_t p_up2 = indata[(y_ - 2) * xsize + x_];
    int32_t p_down2 = indata[(y_ + 2) * xsize + x_];

    int32_t east_estimate = p_right + (p - p_right2) / 2;
    int32_t west_estimate = p_left + (p - p_left2) / 2;
    int32_t north_estimate = p_up + (p - p_up2) / 2;
    int32_t south_estimate = p_down + (p - p_down2) / 2;

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
        out = (uint16_t)dia_estimate;
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

static void rgbir_remosaic_hw_core(uint16_t* src, uint16_t* out0_ptr, uint16_t* out1_ptr, uint32_t xsize, uint32_t ysize,
    bayer_type_t bayer_pattern, const rgbir_remosaic_reg_t* rgbir_remosaic_reg)
{
    int32_t ir_line = 0;
    if(bayer_pattern == RGGIR || bayer_pattern == GRIRG || bayer_pattern == BGGIR || bayer_pattern == GBIRG)
    {
        ir_line = 1;
    }
    // IR up scl
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if(pix_type == PIX_R || pix_type == PIX_B)
            {
                int32_t en_eig = calc_east_north_eig_rb(src, xsize, ysize, col, row);
                int32_t wn_eig = calc_west_north_eig_rb(src, xsize, ysize, col, row);
                int32_t es_eig = calc_east_south_eig_rb(src, xsize, ysize, col, row);
                int32_t ws_eig = calc_west_south_eig_rb(src, xsize, ysize, col, row);

                uint16_t out_ir = estimate_final_ir_for_rb(src, xsize, ysize, col, row, en_eig, wn_eig, es_eig, ws_eig);
                out1_ptr[row*xsize + col] = out_ir;
            }
            else if(pix_type == PIX_GR || pix_type == PIX_GB)
            {
                int32_t east_eig = calc_east_eig_g(src, xsize, ysize, col, row);
                int32_t west_eig = calc_west_eig_g(src, xsize, ysize, col, row);
                int32_t north_eig = calc_north_eig_g(src, xsize, ysize, col, row);
                int32_t south_eig = calc_south_eig_g(src, xsize, ysize, col, row);
                if(row % 2 == ir_line) //left right ir
                {
                    int32_t ir_up = 0;
                    int32_t ir_down = 0;
                    int32_t ir_left = src[row * xsize + REPEAT4(col - 1, MAX_COL)];
                    int32_t ir_right = src[row * xsize + REPEAT4(col + 1, MAX_COL)];
                    estimate_ir_up_down_for_G(src, out1_ptr, row, col, ysize, xsize, &ir_up, &ir_down);
                    out1_ptr[row*xsize + col] = estimate_final_ir_for_g(src, xsize, ysize, col, row, east_eig, west_eig, south_eig, north_eig,
                        ir_up, ir_down, ir_left, ir_right);
                }
                else{
                    int32_t ir_up = src[REPEAT4(row - 1, MAX_ROW) * xsize + col];
                    int32_t ir_down = src[REPEAT4(row + 1, MAX_ROW) * xsize + col];
                    int32_t ir_left = 0;
                    int32_t ir_right = 0;
                    estimate_ir_left_right_for_G(src, out1_ptr, row, col, ysize, xsize, &ir_left, &ir_right);
                    out1_ptr[row*xsize + col] = estimate_final_ir_for_g(src, xsize, ysize, col, row, east_eig, west_eig, south_eig, north_eig,
                        ir_up, ir_down, ir_left, ir_right);
                }
            }
            else{
                out1_ptr[row*xsize + col] = src[row * xsize + col];
            }
        }
    }

    //raw substract ir
    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            uint16_t ir_val = out1_ptr[row*xsize + col]; //14 bit
            int32_t idx0 = ir_val >> 10;
            int32_t idx1 = idx0 + 1;
            idx1 = CLIP3(idx1, 0, 16);
            int32_t delta = ir_val - (idx0 << 10);
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if(pix_type == PIX_Y)
            {
                continue;
            }
            else if(pix_type == PIX_R)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_r[idx0] + (rgbir_remosaic_reg->ir_subtract_r[idx1] - rgbir_remosaic_reg->ir_subtract_r[idx0]) * delta / 1024;
                int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                src[row*xsize + col] = CLIP3(val, 0, 16383);
            }
            else if(pix_type == PIX_GR || pix_type == PIX_GB)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_g[idx0] + (rgbir_remosaic_reg->ir_subtract_g[idx1] - rgbir_remosaic_reg->ir_subtract_g[idx0]) * delta / 1024;
                int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                src[row*xsize + col] = CLIP3(val, 0, 16383);
            }
            else if(pix_type == PIX_B)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_b[idx0] + (rgbir_remosaic_reg->ir_subtract_b[idx1] - rgbir_remosaic_reg->ir_subtract_b[idx0]) * delta / 1024;
                int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                src[row*xsize + col] = CLIP3(val, 0, 16383);
            }
        }
    }

    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if((bayer_pattern == RGGIR || bayer_pattern == BGGIR || bayer_pattern == IRGGR || bayer_pattern == IRGGB)
                && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4)
                {
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == GRIRG || bayer_pattern == GBIRG) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4 + 1)
                {
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == GIRBG || bayer_pattern == GIRRG) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4 - 1)
                {
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == RGGIR || bayer_pattern == GRIRG || bayer_pattern == IRGGR || bayer_pattern == GIRRG) && pix_type == PIX_B) //replace B with R
            {
                uint16_t R_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t R_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t R_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t R_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                int32_t mean_val = int32_t(R_l + R_r + R_t + R_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
            }
            else if((bayer_pattern == BGGIR || bayer_pattern == GBIRG || bayer_pattern == GIRBG || bayer_pattern == IRGGB) && pix_type == PIX_R) //replace R with B
            {
                uint16_t B_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t B_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t B_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t B_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                int32_t mean_val = int32_t(B_l + B_r + B_t + B_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
            }
            else{
                out0_ptr[row*xsize + col] = src[row * xsize + col];
            }
        }
    }
}

static void rgbir_remosaic_hw_core_blinear(uint16_t* src, uint16_t* out0_ptr, uint16_t* out1_ptr, uint32_t xsize, uint32_t ysize,
    bayer_type_t bayer_pattern, const rgbir_remosaic_reg_t* rgbir_remosaic_reg)
{
    // IR up scl
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if(pix_type == PIX_R || pix_type == PIX_B)
            {
                uint16_t Y_lt = src[FLIP(row - 1, MAX_ROW) * xsize + FLIP(col - 1, MAX_COL)];
                uint16_t Y_rt = src[FLIP(row - 1, MAX_ROW) * xsize + FLIP(col + 1, MAX_COL)];
                uint16_t Y_lb = src[FLIP(row + 1, MAX_ROW) * xsize + FLIP(col - 1, MAX_COL)];
                uint16_t Y_rb = src[FLIP(row + 1, MAX_ROW) * xsize + FLIP(col + 1, MAX_COL)];
                uint32_t mean_val = uint32_t(Y_lt + Y_rt + Y_lb + Y_rb + 2) / 4;
                out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
            }
            else if(pix_type == PIX_GR || pix_type == PIX_GB)
            {
                if(row % 2 == 0)
                {
                    uint16_t Y_t = src[FLIP(row - 1, MAX_ROW) * xsize + col];
                    uint16_t Y_b = src[FLIP(row + 1, MAX_ROW) * xsize + col];
                    uint16_t mean_val = (Y_t + Y_b + 1) / 2;
                    out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_l = src[row * xsize + FLIP(col - 1, MAX_COL)];
                    uint16_t Y_r = src[row * xsize + FLIP(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_l + Y_r + 1) / 2;
                    out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else{
                out1_ptr[row*xsize + col] = src[row * xsize + col];
            }
        }
    }

    //raw substract ir
    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            uint16_t ir_val = out1_ptr[row*xsize + col]; //14 bit
            int32_t idx0 = ir_val >> 10;
            int32_t idx1 = idx0 + 1;
            idx1 = CLIP3(idx1, 0, 16);
            int32_t delta = ir_val - (idx0 << 10);
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if(pix_type == PIX_Y)
            {
                continue;
            }
            else if(pix_type == PIX_R)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_r[idx0] + (rgbir_remosaic_reg->ir_subtract_r[idx1] - rgbir_remosaic_reg->ir_subtract_r[idx0]) * delta / 1024;
                //int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                //src[row*xsize + col] = CLIP3(val, 0, 16383);
                uint16_t left_b = src[row * xsize + REPEAT4(col - 2, MAX_COL)];
                uint16_t right_b = src[row * xsize + REPEAT4(col + 2, MAX_COL)];
                uint16_t top_b = src[REPEAT4(row - 2, MAX_ROW) * xsize + col];
                uint16_t bottom_b = src[REPEAT4(row + 2, MAX_ROW) * xsize + col];
                uint16_t left_g = src[row * xsize + REPEAT4(col - 1, MAX_COL)];
                uint16_t right_g = src[row * xsize + REPEAT4(col + 1, MAX_COL)];
                uint16_t top_g = src[REPEAT4(row - 1, MAX_ROW) * xsize + col];
                uint16_t bottom_g = src[REPEAT4(row + 1, MAX_ROW) * xsize + col];
            }
            else if(pix_type == PIX_GR || pix_type == PIX_GB)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_g[idx0] + (rgbir_remosaic_reg->ir_subtract_g[idx1] - rgbir_remosaic_reg->ir_subtract_g[idx0]) * delta / 1024;
                int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                src[row*xsize + col] = CLIP3(val, 0, 16383);
            }
            else if(pix_type == PIX_B)
            {
                int32_t coeff = rgbir_remosaic_reg->ir_subtract_b[idx0] + (rgbir_remosaic_reg->ir_subtract_b[idx1] - rgbir_remosaic_reg->ir_subtract_b[idx0]) * delta / 1024;
                int32_t val = (int32_t)src[row*xsize + col] - ((coeff * (int32_t)out1_ptr[row*xsize + col] + 512)>>10);

                src[row*xsize + col] = CLIP3(val, 0, 16383);
            }
        }
    }

    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if((bayer_pattern == RGGIR || bayer_pattern == BGGIR || bayer_pattern == IRGGR || bayer_pattern == IRGGB)
                && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4)
                {
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == GRIRG || bayer_pattern == GBIRG) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4 + 1)
                {
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == GIRBG || bayer_pattern == GIRRG) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4 - 1)
                {
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
                else{
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
                }
            }
            else if((bayer_pattern == RGGIR || bayer_pattern == GRIRG || bayer_pattern == IRGGR || bayer_pattern == GIRRG) && pix_type == PIX_B) //replace B with R
            {
                uint16_t R_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t R_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t R_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t R_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                int32_t mean_val = int32_t(R_l + R_r + R_t + R_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
            }
            else if((bayer_pattern == BGGIR || bayer_pattern == GBIRG || bayer_pattern == GIRBG || bayer_pattern == IRGGB) && pix_type == PIX_R) //replace R with B
            {
                uint16_t B_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t B_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t B_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t B_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                int32_t mean_val = int32_t(B_l + B_r + B_t + B_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 16383);
            }
            else{
                out0_ptr[row*xsize + col] = src[row * xsize + col];
            }
        }
    }
}

void rgbir_remosaic_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    if(bayer_pattern != BGGIR && bayer_pattern != RGGIR && 
        bayer_pattern != GRIRG && bayer_pattern != GBIRG &&
        bayer_pattern != IRGGR && bayer_pattern != GIRRG &&
        bayer_pattern != IRGGB && bayer_pattern != GIRBG)
    {
        log_error("rgbir_remosaic_hw not support pattern %d\n", bayer_pattern);
        exit(EXIT_FAILURE);
    }

    if (in.size() > 1)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
        memcpy(rgbir_remosaic_reg, &fe_reg->rgbir_remosaic_reg, sizeof(rgbir_remosaic_reg_t));
    }

    rgbir_remosaic_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        for(int32_t i=0; i<17; i++)
        {
            rgbir_remosaic_reg->ir_subtract_r[i] = ir_subtract_r[i];
            rgbir_remosaic_reg->ir_subtract_g[i] = ir_subtract_g[i];
            rgbir_remosaic_reg->ir_subtract_b[i] = ir_subtract_b[i];
        }
    }
    else{
        for (int32_t i = 0; i < 17; i++)
        {
            ir_subtract_r[i] = rgbir_remosaic_reg->ir_subtract_r[i];
            ir_subtract_g[i] = rgbir_remosaic_reg->ir_subtract_g[i];
            ir_subtract_b[i] = rgbir_remosaic_reg->ir_subtract_b[i];
        }
    }

    checkparameters(rgbir_remosaic_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;
    bayer_type_t output0_bayer;
    if (input_raw->bayer_pattern == BGGIR || input_raw->bayer_pattern == IRGGR)
    {
        output0_bayer = BGGR;
    }
    else if(input_raw->bayer_pattern == RGGIR || input_raw->bayer_pattern == IRGGB)
    {
        output0_bayer = RGGB;
    }
    else if(input_raw->bayer_pattern == GRIRG || input_raw->bayer_pattern == GIRBG)
    {
        output0_bayer = GRBG;
    }
    else if(input_raw->bayer_pattern == GBIRG || input_raw->bayer_pattern == GIRRG)
    {
        output0_bayer = GBRG;
    }
    data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, output0_bayer, "rgbir_remosaic_out0");
    uint16_t* out0_ptr = output0->data_ptr;
    out[0] = output0;
    
    log_debug("input bayer format %d, output bayer format %d\n", bayer_pattern, output0->bayer_pattern);
    data_buffer* output1 = new data_buffer(xsize, ysize, DATA_TYPE_Y, BAYER_UNSUPPORT, "rgbir_remosaic_out1");
    uint16_t* out1_ptr = output1->data_ptr;
    out[1] = output1;

    std::unique_ptr<uint16_t[]> tmp_ptr(new uint16_t[xsize * ysize]);
    uint16_t* tmp = tmp_ptr.get();

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 14);
        out0_ptr[sz] = tmp[sz];
    }

    if (rgbir_remosaic_reg->bypass == 0)
    {
        rgbir_remosaic_hw_core(tmp, out0_ptr, out1_ptr, xsize, ysize, bayer_pattern, rgbir_remosaic_reg);
    }
    else{
        rgbir_remosaic_hw_core_blinear(tmp, out0_ptr, out1_ptr, xsize, ysize, bayer_pattern, rgbir_remosaic_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void rgbir_remosaic_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                  UINT_32,     &this->bypass              },
        {"ir_subtract_r",        VECT_INT32,     &this->ir_subtract_r,    17},
        {"ir_subtract_g",        VECT_INT32,     &this->ir_subtract_g,    17},
        {"ir_subtract_b",        VECT_INT32,     &this->ir_subtract_b,    17}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

rgbir_remosaic_hw::~rgbir_remosaic_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (rgbir_remosaic_reg != nullptr)
    {
        delete rgbir_remosaic_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void rgbir_remosaic_hw::checkparameters(rgbir_remosaic_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for(int32_t i=0; i<17; i++)
    {
        reg->ir_subtract_r[i] = common_check_bits_ex(reg->ir_subtract_r[i], 13, "ir_subtract_r"); //+- 1.0 ir
        reg->ir_subtract_g[i] = common_check_bits_ex(reg->ir_subtract_g[i], 13, "ir_subtract_g"); //+- 1.0 ir
        reg->ir_subtract_b[i] = common_check_bits_ex(reg->ir_subtract_b[i], 13, "ir_subtract_b"); //+- 1.0 ir
    }

    log_info("================= rgbir_remosaic reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_1d_array("ir_subtract_r[17]:\n", "%d, ", reg->ir_subtract_r, 17, 17);
    log_1d_array("ir_subtract_g[17]:\n", "%d, ", reg->ir_subtract_g, 17, 17);
    log_1d_array("ir_subtract_b[17]:\n", "%d, ", reg->ir_subtract_b, 17, 17);
    log_info("================= rgbir_remosaic reg=================\n");
}