#include "fe_firmware.h"

Gamma::Gamma(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    gamma_x1024 = 2253;
}

static void gamma_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const gamma_reg_t* gamma_reg)
{
    double inv_gamma_val = 1024.0 / gamma_reg->gamma_x1024;

    uint16_t max_value = (1U << 14) - 1;
    uint16_t* lut = new uint16_t[max_value + 1];
    for (uint16_t i = 0; i < max_value + 1; i++)
    {
        double tmp = double(i) / max_value;
        tmp = pow(tmp, inv_gamma_val);
        tmp = tmp * max_value;
        uint16_t tmp_i = uint16_t(tmp+0.5);
        if (tmp_i > max_value)
            tmp_i = max_value;
        if (tmp_i < 0)
            tmp_i = 0;
        lut[i] = tmp_i >> 4;
    }

    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            
            out_r[row*xsize + col] = lut[r[row*xsize + col]];
            out_g[row*xsize + col] = lut[g[row*xsize + col]];
            out_b[row*xsize + col] = lut[b[row*xsize + col]];
        }
    }

    delete[] lut;
}

void Gamma::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
    gamma_reg_t* gamma_reg = &fe_reg->gamma_reg;

    gamma_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        gamma_reg->gamma_x1024 = gamma_x1024;
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

    uint16_t* tmp0 = new uint16_t[xsize*ysize];
    uint16_t* tmp1 = new uint16_t[xsize*ysize];
    uint16_t* tmp2 = new uint16_t[xsize*ysize];

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

    delete[] tmp0;
    delete[] tmp1;
    delete[] tmp2;

    hw_base::hw_run(stat_out, frame_cnt);
    spdlog::info("{0} run end", __FUNCTION__);
}

void Gamma::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,         &this->bypass          },
        {"gamma_x1024",            UINT_32,         &this->gamma_x1024     }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}

Gamma::~Gamma()
{
    spdlog::info("{0} module deinit start", __FUNCTION__);
    spdlog::info("{0} module deinit end", __FUNCTION__);
}

void Gamma::checkparameters(gamma_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->gamma_x1024 = common_check_bits(reg->gamma_x1024, 12, "gamma_x1024");
    spdlog::info("================= gamma reg=================");
    spdlog::info("bypass {}", reg->bypass);
    spdlog::info("bypass {}", reg->gamma_x1024);
    spdlog::info("================= gamma reg=================");
}
