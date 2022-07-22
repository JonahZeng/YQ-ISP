#include "pipe_register.h"
#include <assert.h>

#define MAX2(a, b) ((a>b)?a:b)
#define MAX3(a, b, c) MAX2(a, MAX2(b, c))
#define MIN2(a, b) ((a<b)?a:b)
#define MIN3(a, b, c) MIN2(a, MIN2(b, c))

cc_hw::cc_hw(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name),
    bypass(0), cc_reg(new cc_reg_t), ccm(9, 0)
{
    ccm[0] = 1024; 
    ccm[4] = 1024;
    ccm[8] = 1024;
}


static void rgb2hsv(int32_t r_in, int32_t g_in, int32_t b_in, int32_t& h, int32_t& s, int32_t& v)
{
    int32_t max_ = MAX3(r_in, g_in, b_in);
    int32_t min_ = MIN3(r_in, g_in, b_in);
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


static void cc_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const cc_reg_t* cc_reg)
{
    int32_t r_in, g_in, b_in;
    int32_t r_out, g_out, b_out;
    int32_t r_clip, g_clip, b_clip;

    int32_t h_out, s_out, v_out;
    int32_t h_clip, s_clip, v_clip;

    int32_t h_final, s_final, v_final;

    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {

            r_in = r[row*xsize + col];
            g_in = g[row*xsize + col];
            b_in = b[row*xsize + col];
            r_out = (r_in*cc_reg->ccm[0] + g_in * cc_reg->ccm[1] + b_in * cc_reg->ccm[2] + 512) >> 10;
            g_out = (r_in*cc_reg->ccm[3] + g_in * cc_reg->ccm[4] + b_in * cc_reg->ccm[5] + 512) >> 10;
            b_out = (r_in*cc_reg->ccm[6] + g_in * cc_reg->ccm[7] + b_in * cc_reg->ccm[8] + 512) >> 10;

            r_clip = (r_out > 16383) ? 16383 : (r_out < 0 ? 0 : r_out);
            g_clip = (g_out > 16383) ? 16383 : (g_out < 0 ? 0 : g_out);
            b_clip = (b_out > 16383) ? 16383 : (b_out < 0 ? 0 : b_out);

            rgb2hsv(r_out, g_out, b_out, h_out, s_out, v_out);
            rgb2hsv(r_clip, g_clip, b_clip, h_clip, s_clip, v_clip);

            int32_t total_clip = abs(r_out - r_clip) + abs(g_out - g_clip) + abs(b_out - b_clip);
            if (total_clip == 0)
            {
                out_r[row * xsize + col] = (uint16_t)r_clip;
                out_g[row * xsize + col] = (uint16_t)g_clip;
                out_b[row * xsize + col] = (uint16_t)b_clip;
            }
            else {
                h_final = h_out;
                v_final = v_clip;
                s_final = s_out + ((s_clip - s_out) * total_clip) / 2048;

                int32_t r_final, g_final, b_final;

                hsv2rgb(h_final, s_final, v_final, r_final, g_final, b_final);

                out_r[row * xsize + col] = (uint16_t)r_final;
                out_g[row * xsize + col] = (uint16_t)g_final;
                out_b[row * xsize + col] = (uint16_t)b_final;
            } 
        }
    }
}



void cc_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) 
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(cc_reg, &fe_reg->cc_reg, sizeof(cc_reg_t));
    }

    cc_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        for (uint32_t i = 0; i < 9; i++)
        {
            cc_reg->ccm[i] = ccm[i];
        }
    }

    checkparameters(cc_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    data_buffer* output0 = new data_buffer(xsize, ysize, input0->data_type, input0->bayer_pattern, "cc_out0");
    out[0] = output0;
    data_buffer* output1 = new data_buffer(xsize, ysize, input1->data_type, input1->bayer_pattern, "cc_out1");
    out[1] = output1;
    data_buffer* output2 = new data_buffer(xsize, ysize, input2->data_type, input2->bayer_pattern, "cc_out2");
    out[2] = output2;

    uint16_t* out0_ptr = output0->data_ptr;
    uint16_t* out1_ptr = output1->data_ptr;
    uint16_t* out2_ptr = output2->data_ptr;

    std::unique_ptr<uint16_t[]> tmp0_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp0 = tmp0_ptr.get();
    std::unique_ptr<uint16_t[]> tmp1_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp1 = tmp1_ptr.get();
    std::unique_ptr<uint16_t[]> tmp2_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp2 = tmp2_ptr.get();

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp0[sz] = input0->data_ptr[sz] >> (16 - 14);
        out0_ptr[sz] = tmp0[sz];

        tmp1[sz] = input1->data_ptr[sz] >> (16 - 14);
        out1_ptr[sz] = tmp1[sz];

        tmp2[sz] = input2->data_ptr[sz] >> (16 - 14);
        out2_ptr[sz] = tmp2[sz];
    }

    if (cc_reg->bypass == 0)
    {
        cc_hw_core(tmp0, tmp1, tmp2, out0_ptr, out1_ptr, out2_ptr, xsize, ysize, cc_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 14);
        out2_ptr[sz] = out2_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}
void cc_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          },
        {"ccm",                    VECT_INT32,  &this->ccm,           9},

    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

cc_hw::~cc_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (cc_reg != nullptr)
    {
        delete cc_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void cc_hw::checkparameters(cc_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (uint32_t i = 0; i < 9; i++)
    {
        reg->ccm[i] = common_check_bits_ex(reg->ccm[i], 14, "ccm");
    }

    log_info("================= cc reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_1d_array("ccm:\n", "%5d, ", reg->ccm, 9, 3);
    log_info("================= cc reg=================\n");
}
