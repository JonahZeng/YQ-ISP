#include "fe_firmware.h"
#include <assert.h>

#define HSVLUT_MAX2(a, b) ((a>b)?a:b)
#define HSVLUT_MAX3(a, b, c) HSVLUT_MAX2(a, HSVLUT_MAX2(b, c))
#define HSVLUT_MIN2(a, b) ((a<b)?a:b)
#define HSVLUT_MIN3(a, b, c) HSVLUT_MIN2(a, HSVLUT_MIN2(b, c))

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
    float s_step = 1.0f / hsv_lut_reg->twoDSatDivisions;

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
                h_idx1 = (h_idx1 > (int32_t)hsv_lut_reg->twoDHueDivisions - 1) ? hsv_lut_reg->twoDHueDivisions - 1 : h_idx1;
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

    uint16_t* tmp0 = new uint16_t[xsize * ysize];
    uint16_t* tmp1 = new uint16_t[xsize * ysize];
    uint16_t* tmp2 = new uint16_t[xsize * ysize];

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

    delete[] tmp0;
    delete[] tmp1;
    delete[] tmp2;

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void hsv_lut::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
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


    log_info("================= hs2dlut reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("================= hs2dlut reg=================\n");
}