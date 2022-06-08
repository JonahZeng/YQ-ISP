#include "fe_firmware.h"
#include <assert.h>

#define HSVLUT_MAX2(a, b) ((a>b)?a:b)
#define HSVLUT_MAX3(a, b, c) HSVLUT_MAX2(a, HSVLUT_MAX2(b, c))
#define HSVLUT_MIN2(a, b) ((a<b)?a:b)
#define HSVLUT_MIN3(a, b, c) HSVLUT_MIN2(a, HSVLUT_MIN2(b, c))

#define HSVLUT_CLIP3(in, a, b) (in<a)?a:((in>b)?b:in)

hsv_lut::hsv_lut(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name),
    bypass(0), hsv_lut_reg(new hsv_lut_reg_t)
{
}

static void rgb2hsv(int32_t r_in, int32_t g_in, int32_t b_in, int32_t& h, int32_t& s, int32_t& v)
{
    int32_t max_ = HSVLUT_MAX3(r_in, g_in, b_in);
    int32_t min_ = HSVLUT_MIN3(r_in, g_in, b_in);
    int32_t s_ = 0;
    if (max_ > 0)
    {
        s_ = (max_ - min_) * 16384 / max_;
    }
    int32_t h_ = 0;
    if (max_ == min_)
    {
        h_ = 0;
    }
    else if (max_ == r_in)
    {
        h_ = (g_in - b_in) * 2730 / (max_ - min_);
    }
    else if (max_ == g_in)
    {
        h_ = (b_in - r_in) * 2730 / (max_ - min_) + 5461;
    }
    else {
        h_ = (r_in - g_in) * 2730 / (max_ - min_) + 10923;
    }

    if (h_ < 0)
    {
        h_ = h_ + 16384;
    }

    h = h_;
    s = s_;
    v = max_;
}

static void hsv2rgb(int32_t h, int32_t s, int32_t v, int32_t& r, int32_t& g, int32_t& b)
{
    if (h >= 0 && h < 2730)
    {
        r = v;
        b = (16384 - s) * r / 16384;
        g = (r - b) * h / 2730 + b;
    }
    else if (h >= 2730 && h < 5461)
    {
        g = v;
        b = (16384 - s) * g / 16384;
        r = b - (g - b) * (h - 5461) / 2730;
    }
    if (h >= 5461 && h < 8192)
    {
        g = v;
        r = (16384 - s) * g / 16384;
        b = (g - r) * (h - 5461) / 2730 + r;
    }
    if (h >= 8192 && h < 10922)
    {
        b = v;
        r = (16384 - s) * b / 16384;
        g = r - (b - r) * (h - 10922) / 2730;
    }
    if (h >= 10922 && h < 13653)
    {
        b = v;
        g = (16384 - s) * b / 16384;
        r = (b - g) * (h - 10922) / 2730 + g;
    }
    if (h >= 13653 && h < 16384)
    {
        r = v;
        g = (16384 - s) * r / 16384;
        b = g - (h - 16384) * (r - g) / 2730;
    }
    r = (r > 16383) ? 16383 : (r < 0 ? 0 : r);
    g = (g > 16383) ? 16383 : (g < 0 ? 0 : g);
    b = (b > 16383) ? 16383 : (b < 0 ? 0 : b);
}

static void hsv_lut_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const hsv_lut_reg_t* hsv_lut_reg)
{
    int32_t r_in, g_in, b_in;
    int32_t h_in, s_in, v_in;
    int32_t r_out, g_out, b_out;
    float h_step = 360.0f / hsv_lut_reg->twoDHueDivisions;
    float s_step = 1.0f / (hsv_lut_reg->twoDSatDivisions - 1);
    float v_step = 1.0f;

    const dng_hue_sat_map::HSBModify* twoDMap = hsv_lut_reg->twoDmap;
    
    if (hsv_lut_reg->twoD_enable)
    {
        for (uint32_t row = 0; row < ysize; row++)
        {
            for (uint32_t col = 0; col < xsize; col++)
            {
                r_in = r[row * xsize + col];
                g_in = g[row * xsize + col];
                b_in = b[row * xsize + col];

                rgb2hsv(r_in, g_in, b_in, h_in, s_in, v_in);

                float h_ = h_in * 360.0f / 16384.0f;
                float s_ = s_in / 16384.0f;
                //float v_ = v_in / 16383.0f;

                int32_t h_idx0 = (int32_t)floorf(h_ / h_step);
                int32_t h_idx1 = h_idx0 + 1;
                h_idx1 = (h_idx1 > (int32_t)hsv_lut_reg->twoDHueDivisions - 1) ? 0 : h_idx1;
                float h_delta = h_ - h_idx0 * h_step;
                int32_t s_idx0 = (int32_t)floorf(s_ / s_step);
                int32_t s_idx1 = s_idx0 + 1;
                s_idx1 = (s_idx1 > (int32_t)hsv_lut_reg->twoDSatDivisions - 1) ? hsv_lut_reg->twoDSatDivisions - 1 : s_idx1;
                float s_delta = s_ - s_idx0 * s_step;

                float h_oft00 = twoDMap[h_idx0 * hsv_lut_reg->twoDSatDivisions + s_idx0].fHueShift;
                float h_oft01 = twoDMap[h_idx0 * hsv_lut_reg->twoDSatDivisions + s_idx1].fHueShift;
                float h_oft10 = twoDMap[h_idx1 * hsv_lut_reg->twoDSatDivisions + s_idx0].fHueShift;
                float h_oft11 = twoDMap[h_idx1 * hsv_lut_reg->twoDSatDivisions + s_idx1].fHueShift;

                float s_oft00 = twoDMap[h_idx0 * hsv_lut_reg->twoDSatDivisions + s_idx0].fSatScale;
                float s_oft01 = twoDMap[h_idx0 * hsv_lut_reg->twoDSatDivisions + s_idx1].fSatScale;
                float s_oft10 = twoDMap[h_idx1 * hsv_lut_reg->twoDSatDivisions + s_idx0].fSatScale;
                float s_oft11 = twoDMap[h_idx1 * hsv_lut_reg->twoDSatDivisions + s_idx1].fSatScale;

                float h_oft0 = h_oft00 + (h_oft01 - h_oft00) * s_delta / s_step;
                float h_oft1 = h_oft10 + (h_oft11 - h_oft10) * s_delta / s_step;
                float h_oft = h_oft0 + (h_oft1 - h_oft0) * h_delta / h_step;
                h_ += h_oft;
                if (h_ > 360.0f)
                {
                    h_ = h_ - 360.0f;
                }
                h_ = h_ * 16384.0f / 360.0f;

                float s_oft0 = s_oft00 + (s_oft01 - s_oft00) * s_delta / s_step;
                float s_oft1 = s_oft10 + (s_oft11 - s_oft10) * s_delta / s_step;
                float s_oft = s_oft0 + (s_oft1 - s_oft0) * h_delta / h_step;
                s_ = s_ * s_oft;
                s_ = s_ * 16384.0f;

                int32_t h_final = (int32_t)floorf(h_ + 0.5f);
                h_final = h_final > 16384 ? 16384 : h_final;
                int32_t s_final = (int32_t)floorf(s_ + 0.5f);
                s_final = s_final > 16384 ? 16384 : s_final;

                hsv2rgb(h_final, s_final, v_in, r_out, g_out, b_out);

                r_out = HSVLUT_CLIP3(r_out, 0, 16383);
                g_out = HSVLUT_CLIP3(g_out, 0, 16383);
                b_out = HSVLUT_CLIP3(b_out, 0, 16383);

                out_r[row * xsize + col] = r_out;
                out_g[row * xsize + col] = g_out;
                out_b[row * xsize + col] = b_out;
            }
        }
    }
    else
    {
        for (uint32_t row = 0; row < ysize; row++)
        {
            for (uint32_t col = 0; col < xsize; col++)
            {
                out_r[row * xsize + col] = r[row * xsize + col];
                out_g[row * xsize + col] = g[row * xsize + col];
                out_b[row * xsize + col] = b[row * xsize + col];
            }
        }
    }

    const dng_hue_sat_map::HSBModify* threeDMap = hsv_lut_reg->threeDmap; //order v-h-s
    uint32_t hs_size = hsv_lut_reg->threeDHueDivisions * hsv_lut_reg->threeDSatDivisions;
    uint32_t s_size = hsv_lut_reg->threeDSatDivisions;
    if (hsv_lut_reg->threeD_enable)
    {
        h_step = 360.0f / hsv_lut_reg->threeDHueDivisions;
        s_step = 1.0f / (hsv_lut_reg->threeDSatDivisions - 1);
        v_step = 1.0f / (hsv_lut_reg->threeDValDivisions - 1);
        for (uint32_t row = 0; row < ysize; row++)
        {
            for (uint32_t col = 0; col < xsize; col++)
            {
                r_in = out_r[row * xsize + col];
                g_in = out_g[row * xsize + col];
                b_in = out_b[row * xsize + col];

                rgb2hsv(r_in, g_in, b_in, h_in, s_in, v_in);

                float h_ = h_in * 360.0f / 16384.0f;
                float s_ = s_in / 16384.0f;
                float v_ = v_in / 16383.0f;

                int32_t v_idx0 = (int32_t)floorf(v_ / v_step);
                int32_t v_idx1 = v_idx0 + 1;
                v_idx1 = (v_idx1 > (int32_t)hsv_lut_reg->threeDValDivisions - 1) ? hsv_lut_reg->threeDValDivisions - 1 : v_idx1;
                float v_delta = v_ - v_idx0 * v_step;
                v_delta = v_delta / v_step;
                int32_t h_idx0 = (int32_t)floorf(h_ / h_step);
                int32_t h_idx1 = h_idx0 + 1;
                h_idx1 = (h_idx1 > (int32_t)hsv_lut_reg->threeDHueDivisions - 1) ? 0 : h_idx1;
                float h_delta = h_ - h_idx0 * h_step;
                h_delta = h_delta / h_step;
                int32_t s_idx0 = (int32_t)floorf(s_ / s_step);
                int32_t s_idx1 = s_idx0 + 1;
                s_idx1 = (s_idx1 > (int32_t)hsv_lut_reg->threeDSatDivisions - 1) ? hsv_lut_reg->threeDSatDivisions - 1 : s_idx1;
                float s_delta = s_ - s_idx0 * s_step;
                s_delta = s_delta / s_step;

                float h_oft000 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx0].fHueShift;
                float h_oft001 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx1].fHueShift;
                float h_oft010 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx0].fHueShift;
                float h_oft011 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx1].fHueShift;
                float h_oft100 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx0].fHueShift;
                float h_oft101 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx1].fHueShift;
                float h_oft110 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx0].fHueShift;
                float h_oft111 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx1].fHueShift;

                float s_oft000 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx0].fSatScale;
                float s_oft001 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx1].fSatScale;
                float s_oft010 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx0].fSatScale;
                float s_oft011 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx1].fSatScale;
                float s_oft100 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx0].fSatScale;
                float s_oft101 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx1].fSatScale;
                float s_oft110 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx0].fSatScale;
                float s_oft111 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx1].fSatScale;

                float v_oft000 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx0].fValScale;
                float v_oft001 = threeDMap[v_idx0 * hs_size + h_idx0 * s_size + s_idx1].fValScale;
                float v_oft010 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx0].fValScale;
                float v_oft011 = threeDMap[v_idx0 * hs_size + h_idx1 * s_size + s_idx1].fValScale;
                float v_oft100 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx0].fValScale;
                float v_oft101 = threeDMap[v_idx1 * hs_size + h_idx0 * s_size + s_idx1].fValScale;
                float v_oft110 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx0].fValScale;
                float v_oft111 = threeDMap[v_idx1 * hs_size + h_idx1 * s_size + s_idx1].fValScale;

                //S0V0H0
                //S0V1H0
                //S1V0H0
                //S1V1H0
                //S0V0H1
                //S0V1H1
                //S1V0H1
                //S1V1H1
                float V[8][3] = { 
                    {s_oft000, v_oft000, h_oft000},
                    {s_oft100, v_oft100, h_oft100},
                    {s_oft001, v_oft001, h_oft001},
                    {s_oft101, v_oft101, h_oft101},
                    {s_oft010, v_oft010, h_oft010},
                    {s_oft110, v_oft110, h_oft110},
                    {s_oft011, v_oft011, h_oft011},
                    {s_oft111, v_oft111, h_oft111} };
                float delta_t[4] = {1.0f, h_delta, s_delta, v_delta};

                float t00 = 0.0, t01 = 0.0, t02 = 0.0;
                float t10 = 0.0, t11 = 0.0, t12 = 0.0;
                float t20 = 0.0, t21 = 0.0, t22 = 0.0;
                float t30 = 0.0, t31 = 0.0, t32 = 0.0;

                if (h_delta >= s_delta && s_delta >= v_delta)
                {
                    // 1 0 0 0  0 0  0 0
                    //−1 0 0 0  1 0  0 0
                    // 0 0 0 0 −1 0  1 0
                    // 0 0 0 0  0 0 −1 1
                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[4][0] - V[0][0];
                    t11 = V[4][1] - V[0][1];
                    t12 = V[4][2] - V[0][2];

                    t20 = V[6][0] - V[4][0];
                    t21 = V[6][1] - V[4][1];
                    t22 = V[6][2] - V[4][2];

                    t30 = V[7][0] - V[6][0];
                    t31 = V[7][1] - V[6][1];
                    t32 = V[7][2] - V[6][2];
                }
                else if (h_delta >= v_delta && v_delta >= s_delta)
                {
                    //1 0 0 0 0 0 0 0
                    //−1 0 0 0 1 0 0 0
                    //0 0 0 0 0 −1 0 1
                    //0 0 0 0 −1 1 0 0

                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[4][0] - V[0][0];
                    t11 = V[4][1] - V[0][1];
                    t12 = V[4][2] - V[0][2];

                    t20 = V[7][0] - V[5][0];
                    t21 = V[7][1] - V[5][1];
                    t22 = V[7][2] - V[5][2];

                    t30 = V[5][0] - V[4][0];
                    t31 = V[5][1] - V[4][1];
                    t32 = V[5][2] - V[4][2];
                }
                else if (v_delta >= h_delta && h_delta >= s_delta)
                {
                    //1 0 0 0 0 0 0 0
                    //0 −1 0 0 0 1 0 0
                    //0 0 0 0 0 −1 0 1
                    //−1 1 0 0 0 0 0 0
                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[5][0] - V[1][0];
                    t11 = V[5][1] - V[1][1];
                    t12 = V[5][2] - V[1][2];

                    t20 = V[7][0] - V[5][0];
                    t21 = V[7][1] - V[5][1];
                    t22 = V[7][2] - V[5][2];

                    t30 = V[1][0] - V[0][0];
                    t31 = V[1][1] - V[0][1];
                    t32 = V[1][2] - V[0][2];
                }
                else if (s_delta >= h_delta && h_delta >= v_delta)
                {
                    //1 0 0 0 0 0 0 0
                    //0 0 −1 0 0 0 1 0
                    //−1 0 1 0 0 0 0 0
                    //0 0 0 0 0 0 −1 1
                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[6][0] - V[2][0];
                    t11 = V[6][1] - V[2][1];
                    t12 = V[6][2] - V[2][2];

                    t20 = V[2][0] - V[0][0];
                    t21 = V[2][1] - V[0][1];
                    t22 = V[2][2] - V[0][2];

                    t30 = V[7][0] - V[6][0];
                    t31 = V[7][1] - V[6][1];
                    t32 = V[7][2] - V[6][2];
                }
                else if (s_delta >= v_delta && v_delta >= h_delta)
                {
                    //1 0 0 0 0 0 0 0
                    //0 0 0 −1 0 0 0 1
                    //−1 0 1 0 0 0 0 0
                    //0 0 −1 1 0 0 0 0
                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[7][0] - V[3][0];
                    t11 = V[7][1] - V[3][1];
                    t12 = V[7][2] - V[3][2];

                    t20 = V[2][0] - V[0][0];
                    t21 = V[2][1] - V[0][1];
                    t22 = V[2][2] - V[0][2];

                    t30 = V[3][0] - V[2][0];
                    t31 = V[3][1] - V[2][1];
                    t32 = V[3][2] - V[2][2];
                }
                else if (v_delta >= s_delta && s_delta >= h_delta)
                {
                    //1 0 0 0 0 0 0 0
                    //0 0 0 −1 0 0 0 1
                    //0 −1 0 1 0 0 0 0
                    //−1 1 0 0 0 0 0 0
                    t00 = V[0][0];
                    t01 = V[0][1];
                    t02 = V[0][2];

                    t10 = V[7][0] - V[3][0];
                    t11 = V[7][1] - V[3][1];
                    t12 = V[7][2] - V[3][2];

                    t20 = V[3][0] - V[1][0];
                    t21 = V[3][1] - V[1][1];
                    t22 = V[3][2] - V[1][2];

                    t30 = V[1][0] - V[0][0];
                    t31 = V[1][1] - V[0][1];
                    t32 = V[1][2] - V[0][2];
                }
                else
                {
                    log_error("3dlut hw can't cover all pixel\n");
                }

                float s_oft = delta_t[0] * t00 + delta_t[1] * t10 + delta_t[2] * t20 + delta_t[3] * t30;
                float v_oft = delta_t[0] * t01 + delta_t[1] * t11 + delta_t[2] * t21 + delta_t[3] * t31;
                float h_oft = delta_t[0] * t02 + delta_t[1] * t12 + delta_t[2] * t22 + delta_t[3] * t32;

                h_ += h_oft;
                if (h_ > 360.0f)
                {
                    h_ = h_ - 360.0f;
                }
                h_ = h_ * 16384.0f / 360.0f;

                s_ = s_ * s_oft;
                s_ = s_ * 16384.0f;

                v_ = v_ * v_oft;
                v_ = v_ * 16383.0f;

                int32_t h_final = (int32_t)floorf(h_ + 0.5f);
                h_final = HSVLUT_CLIP3(h_final, 0, 16384);
                int32_t s_final = (int32_t)floorf(s_ + 0.5f);
                s_final = HSVLUT_CLIP3(s_final, 0, 16384);
                int32_t v_final = (int32_t)floorf(v_ + 0.5f);
                v_final = HSVLUT_CLIP3(v_final, 0, 16383);

                hsv2rgb(h_final, s_final, v_in, r_out, g_out, b_out);

                r_out = HSVLUT_CLIP3(r_out, 0, 16383);
                g_out = HSVLUT_CLIP3(g_out, 0, 16383);
                b_out = HSVLUT_CLIP3(b_out, 0, 16383);

                out_r[row * xsize + col] = r_out;
                out_g[row * xsize + col] = g_out;
                out_b[row * xsize + col] = b_out;
            }
        }
    }
}

void hsv_lut::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(hsv_lut_reg, &fe_reg->hsv_lut_reg, sizeof(hsv_lut_reg_t));
    }

    hsv_lut_reg->bypass = bypass;
    if (xmlConfigValid)
    {

    }

    checkparameters(hsv_lut_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    data_buffer* output0 = new data_buffer(xsize, ysize, input0->data_type, input0->bayer_pattern, "hs2dlut_out0");
    out[0] = output0;
    data_buffer* output1 = new data_buffer(xsize, ysize, input1->data_type, input1->bayer_pattern, "hs2dlut_out1");
    out[1] = output1;
    data_buffer* output2 = new data_buffer(xsize, ysize, input2->data_type, input2->bayer_pattern, "hs2dlut_out2");
    out[2] = output2;

    uint16_t* out0_ptr = output0->data_ptr;
    uint16_t* out1_ptr = output1->data_ptr;
    uint16_t* out2_ptr = output2->data_ptr;

    std::unique_ptr<uint16_t[]> tmp0_ptr(new uint16_t[xsize * ysize]);
    uint16_t* tmp0 = tmp0_ptr.get();
    std::unique_ptr<uint16_t[]> tmp1_ptr(new uint16_t[xsize * ysize]);
    uint16_t* tmp1 = tmp1_ptr.get();
    std::unique_ptr<uint16_t[]> tmp2_ptr(new uint16_t[xsize * ysize]);
    uint16_t* tmp2 = tmp2_ptr.get();

    for (uint32_t sz = 0; sz < xsize * ysize; sz++)
    {
        tmp0[sz] = input0->data_ptr[sz] >> (16 - 14);
        out0_ptr[sz] = tmp0[sz];

        tmp1[sz] = input1->data_ptr[sz] >> (16 - 14);
        out1_ptr[sz] = tmp1[sz];

        tmp2[sz] = input2->data_ptr[sz] >> (16 - 14);
        out2_ptr[sz] = tmp2[sz];
    }

    if (hsv_lut_reg->bypass == 0)
    {
        hsv_lut_hw_core(tmp0, tmp1, tmp2, out0_ptr, out1_ptr, out2_ptr, xsize, ysize, hsv_lut_reg);
    }

    for (uint32_t sz = 0; sz < xsize * ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 14);
        out2_ptr[sz] = out2_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void hsv_lut::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

hsv_lut::~hsv_lut()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (hsv_lut_reg != nullptr)
    {
        delete hsv_lut_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}


void hsv_lut::checkparameters(hsv_lut_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->twoD_enable = common_check_bits(reg->twoD_enable, 1, "twoD_enable");
    reg->threeD_enable = common_check_bits(reg->threeD_enable, 1, "threeD_enable");
    reg->twoD_map_encoding = common_check_bits(reg->twoD_map_encoding, 1, "twoD_map_encoding");
    reg->threeD_map_encoding = common_check_bits(reg->threeD_map_encoding, 1, "threeD_map_encoding");
    reg->twoDHueDivisions = common_check_bits(reg->twoDHueDivisions, 7, "twoDHueDivisions");
    reg->twoDSatDivisions = common_check_bits(reg->twoDSatDivisions, 7, "twoDSatDivisions");
    reg->threeDHueDivisions = common_check_bits(reg->threeDHueDivisions, 7, "threeDHueDivisions");
    reg->threeDSatDivisions = common_check_bits(reg->threeDSatDivisions, 7, "threeDSatDivisions");
    reg->threeDValDivisions = common_check_bits(reg->threeDValDivisions, 7, "threeDValDivisions");

    log_info("================= hs2dlut reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("twoD_enable %d\n", reg->twoD_enable);
    log_info("threeD_enable %d\n", reg->threeD_enable);
    log_info("twoD_map_encoding %d\n", reg->twoD_map_encoding);
    log_info("threeD_map_encoding %d\n", reg->threeD_map_encoding);

    log_info("twoDHueDivisions %d\n", reg->twoDHueDivisions);
    log_info("twoDSatDivisions %d\n", reg->twoDSatDivisions);

    log_info("threeDHueDivisions %d\n", reg->threeDHueDivisions);
    log_info("threeDSatDivisions %d\n", reg->threeDSatDivisions);
    log_info("threeDValDivisions %d\n", reg->threeDValDivisions);
    log_info("================= hs2dlut reg=================\n");
}