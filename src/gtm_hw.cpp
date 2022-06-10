#include "pipe_register.h"
#include <assert.h>

gtm_hw::gtm_hw(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    gain_lut.resize(257);
    gtm_reg = new gtm_reg_t;
}

static void gtm_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const gtm_reg_t* gtm_reg)
{
    const uint32_t* rgb2y = gtm_reg->rgb2y;
    const uint32_t* gain_lut = gtm_reg->gain_lut;
    uint32_t y_tmp = 0;
    int32_t gain_tmp;
    const uint32_t MAX_VAL = (1U << 14) - 1;

    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            y_tmp = r[row * xsize + col] * rgb2y[0] + g[row * xsize + col] * rgb2y[1] + b[row * xsize + col] * rgb2y[2];
            y_tmp = (y_tmp + 512) >> 10;

            if (y_tmp > MAX_VAL)
                y_tmp = MAX_VAL;
            uint32_t idx1 = y_tmp >> 6;
            uint32_t idx2 = idx1 + 1;
            if (idx2 > 256)
                idx2 = 256;
            int32_t delta = y_tmp - idx1 * 64;
            int32_t gain1 = int32_t(gain_lut[idx1]);
            int32_t gain2 = int32_t(gain_lut[idx2]);
            gain_tmp = gain1 + delta * (gain2 - gain1) / 64;

            uint32_t b_out = (b[row * xsize + col] * gain_tmp + 512) >> 10;
            if (b_out > MAX_VAL)
                b_out = MAX_VAL;
            out_b[row * xsize + col] = uint16_t(b_out);
            uint32_t g_out = (g[row * xsize + col] * gain_tmp + 512) >> 10;
            if (g_out > MAX_VAL)
                g_out = MAX_VAL;
            out_g[row * xsize + col] = uint16_t(g_out);
            uint32_t r_out = (r[row * xsize + col] * gain_tmp + 512) >> 10;
            if (r_out > MAX_VAL)
                r_out = MAX_VAL;
            out_r[row * xsize + col] = uint16_t(r_out);
        }
    }
}

void gtm_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];
    
    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(gtm_reg, &fe_reg->gtm_reg, sizeof(gtm_reg_t));
    }

    gtm_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        gtm_reg->rgb2y[0] = rgb2y[0];
        gtm_reg->rgb2y[1] = rgb2y[1];
        gtm_reg->rgb2y[2] = rgb2y[2];

        for (int32_t i = 0; i < 257; i++)
        {
            gtm_reg->gain_lut[i] = gain_lut[i];
        }
    }

    checkparameters(gtm_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    data_buffer* output0 = new data_buffer(xsize, ysize, input0->data_type, input0->bayer_pattern, "gtm_out0");
    out[0] = output0;
    data_buffer* output1 = new data_buffer(xsize, ysize, input1->data_type, input1->bayer_pattern, "gtm_out1");
    out[1] = output1;
    data_buffer* output2 = new data_buffer(xsize, ysize, input2->data_type, input2->bayer_pattern, "gtm_out2");
    out[2] = output2;

    uint16_t* out0_ptr = output0->data_ptr;
    uint16_t* out1_ptr = output1->data_ptr;
    uint16_t* out2_ptr = output2->data_ptr;

    std::unique_ptr<uint16_t[]> tmp0_ptr(new uint16_t[xsize*ysize]);
    std::unique_ptr<uint16_t[]> tmp1_ptr(new uint16_t[xsize*ysize]);
    std::unique_ptr<uint16_t[]> tmp2_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp0 = tmp0_ptr.get();
    uint16_t* tmp1 = tmp1_ptr.get();
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

    if (gtm_reg->bypass == 0)
    {
        gtm_hw_core(tmp0, tmp1, tmp2, out0_ptr, out1_ptr, out2_ptr, xsize, ysize, gtm_reg);
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

void gtm_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,         &this->bypass          },
        {"rgb2y",                  VECT_UINT32,     &this->rgb2y,         3},
        {"tone_curve",             VECT_UINT32,     &this->gain_lut,  257}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

gtm_hw::~gtm_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (gtm_reg != nullptr)
    {
        delete gtm_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void gtm_hw::checkparameters(gtm_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (int32_t i = 0; i < 3; i++)
    {
        reg->rgb2y[i] = common_check_bits(reg->rgb2y[i], 10, "rgb2y");
    }
    for (int32_t i = 0; i < 257; i++)
    {
        reg->gain_lut[i] = common_check_bits(reg->gain_lut[i], 13, "tone_curve"); //1024 = 1.0
    }

    log_info("================= gtm reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("rgb2y %d %d %d\n", reg->rgb2y[0], reg->rgb2y[1], reg->rgb2y[2]);
    log_array("gain_lut:\n", "%6d, ", reg->gain_lut, 257, 16);
    log_info("================= gtm reg=================\n");
}