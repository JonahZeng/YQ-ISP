#include "pipe_register.h"
#include <cmath>
#include <assert.h>

Gamma_hw::Gamma_hw(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name), gamma_lut(257)
{
    bypass = 0;
    gamma_reg = new gamma_reg_t;
}

static void gamma_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const gamma_reg_t* gamma_reg)
{
    const uint32_t* gamma_lut = gamma_reg->gamma_lut;

    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            uint16_t in_r = r[row * xsize + col];
            uint16_t in_g = g[row * xsize + col];
            uint16_t in_b = b[row * xsize + col];

            uint16_t idx0 = in_r >> 6;
            uint16_t idx1 = idx0 + 1;
            idx1 = (idx1 > 256) ? 256 : idx1;
            uint32_t delta = in_r - (idx0 << 6);
            uint16_t o_r = gamma_lut[idx0] + (((uint32_t)(gamma_lut[idx1] - gamma_lut[idx0]) * delta + 32) >> 6);
            o_r = (o_r < 0) ? 0 : ((o_r > 16383) ? 16383 : o_r);

            idx0 = in_g >> 6;
            idx1 = idx0 + 1;
            idx1 = (idx1 > 256) ? 256 : idx1;
            delta = in_g - (idx0 << 6);
            uint16_t o_g = gamma_lut[idx0] + (((uint32_t)(gamma_lut[idx1] - gamma_lut[idx0]) * delta + 32) >> 6);
            o_g = (o_g < 0) ? 0 : ((o_g > 16383) ? 16383 : o_g);

            idx0 = in_b >> 6;
            idx1 = idx0 + 1;
            idx1 = (idx1 > 256) ? 256 : idx1;
            delta = in_b - (idx0 << 6);
            uint16_t o_b = gamma_lut[idx0] + (((uint32_t)(gamma_lut[idx1] - gamma_lut[idx0]) * delta + 32) >> 6);
            o_b = (o_b < 0) ? 0 : ((o_b > 16383) ? 16383 : o_b);

            out_r[row * xsize + col] = o_r >> 4;
            out_g[row * xsize + col] = o_g >> 4;
            out_b[row * xsize + col] = o_b >> 4;
        }
    }
}

void Gamma_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(gamma_reg, &fe_reg->gamma_reg, sizeof(gamma_reg_t));
    }

    gamma_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        for(int32_t i=0; i<257; i++)
        {
            gamma_reg->gamma_lut[i] = gamma_lut[i];
        }
    }

    checkparameters(gamma_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    data_buffer* output0 = new data_buffer(xsize, ysize, input0->data_type, input0->bayer_pattern, "gamma_out0");
    out[0] = output0;
    data_buffer* output1 = new data_buffer(xsize, ysize, input1->data_type, input1->bayer_pattern, "gamma_out1");
    out[1] = output1;
    data_buffer* output2 = new data_buffer(xsize, ysize, input2->data_type, input2->bayer_pattern, "gamma_out2");
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

    if (gamma_reg->bypass == 0)
    {
        gamma_hw_core(tmp0, tmp1, tmp2, out0_ptr, out1_ptr, out2_ptr, xsize, ysize, gamma_reg);
    }
    else {
        for (uint32_t sz = 0; sz < xsize*ysize; sz++)
        {
            out0_ptr[sz] = out0_ptr[sz] >> 4;

            out1_ptr[sz] = out1_ptr[sz] >> 4;

            out2_ptr[sz] = out2_ptr[sz] >> 4;
        }
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 10);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 10);
        out2_ptr[sz] = out2_ptr[sz] << (16 - 10);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void Gamma_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,         &this->bypass          },
        {"gamma_lut",              VECT_UINT32,     &this->gamma_lut,   257}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

Gamma_hw::~Gamma_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (gamma_reg != nullptr)
    {
        delete gamma_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void Gamma_hw::checkparameters(gamma_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for(int32_t i=0; i<257; i++)
    {
        reg->gamma_lut[i] = common_check_bits(reg->gamma_lut[i], 14, "gamma_lut");
    }
    log_info("================= gamma reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_1d_array("gamma_lut:\n", "%5d, ", reg->gamma_lut, 257, 16);
    log_info("================= gamma reg=================\n");
}
