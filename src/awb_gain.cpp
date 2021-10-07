#include "fe_firmware.h"

awbgain::awbgain(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    r_gain = 1024;
    gr_gain = 1024;
    gb_gain = 1024;
    b_gain = 1024;
}

static void awbgain_hw_core(uint16_t* indata, uint16_t* outdata, uint32_t xsize, uint32_t ysize, bayer_type_t by, const awbgain_reg_t* awbgain_reg)
{
    uint32_t gain_val[4];
    if (by == RGGB)
    {
        gain_val[0] = awbgain_reg->r_gain;
        gain_val[1] = awbgain_reg->gr_gain;
        gain_val[2] = awbgain_reg->gb_gain;
        gain_val[3] = awbgain_reg->b_gain;
    }
    else if (by == GRBG)
    {
        gain_val[0] = awbgain_reg->gr_gain;
        gain_val[1] = awbgain_reg->r_gain;
        gain_val[2] = awbgain_reg->b_gain;
        gain_val[3] = awbgain_reg->gb_gain;
    }
    else if (by == GBRG)
    {
        gain_val[0] = awbgain_reg->gb_gain;
        gain_val[1] = awbgain_reg->b_gain;
        gain_val[2] = awbgain_reg->r_gain;
        gain_val[3] = awbgain_reg->gr_gain;
    }
    else if (by == BGGR)
    {
        gain_val[0] = awbgain_reg->b_gain;
        gain_val[1] = awbgain_reg->gb_gain;
        gain_val[2] = awbgain_reg->gr_gain;
        gain_val[3] = awbgain_reg->r_gain;
    }

    for (uint32_t y = 0; y < ysize; y++)
    {
        for (uint32_t x = 0; x < xsize; x++)
        {
            uint32_t pix = indata[y*xsize + x];
            uint32_t channel = ((y % 2) << 1) | (x % 2);
            if (channel > 3)
            {
                spdlog::error("error for blc pix bayer select");
                channel = 3;
            }
            uint32_t gain = gain_val[channel];

            pix = (pix * gain + 512) >> 10;
            pix = (pix < 0) ? 0 : ((pix > 16383) ? 16383 : pix);

            outdata[y*xsize + x] = (uint16_t)pix;
        }
    }
}


void awbgain::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;
    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
    awbgain_reg_t* awbgain_reg = &fe_reg->awbgain_reg;

    awbgain_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        awbgain_reg->r_gain = r_gain;
        awbgain_reg->gr_gain = gr_gain;
        awbgain_reg->gb_gain = gb_gain;
        awbgain_reg->b_gain = b_gain;
    }

    checkparameters(awbgain_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, "awbgain_out0");
    uint16_t* out0_ptr = output0->data_ptr;
    out[0] = output0;

    uint16_t* tmp = new uint16_t[xsize*ysize];

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 14); //nikon d610 14bit capture raw, right shift to 14bit precision
        out0_ptr[sz] = tmp[sz];
    }

    if (awbgain_reg->bypass == 0)
    {
        awbgain_hw_core(tmp, out0_ptr, xsize, ysize, bayer_pattern, awbgain_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14); //nikon d610 14bit capture raw, back to high bits
    }

    delete[] tmp;

    hw_base::hw_run(stat_out, frame_cnt);
    spdlog::info("{0} run end", __FUNCTION__);
}
void awbgain::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          },
        {"r_gain",                 UINT_32,     &this->r_gain          },
        {"gr_gain",                UINT_32,     &this->gr_gain         },
        {"gb_gain",                UINT_32,     &this->gb_gain         },
        {"b_gain",                 UINT_32,     &this->b_gain          },
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}
awbgain::~awbgain()
{
    spdlog::info("{0} module deinit start", __FUNCTION__);
    spdlog::info("{0} module deinit end", __FUNCTION__);
}


void awbgain::checkparameters(awbgain_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->r_gain = common_check_bits(reg->r_gain, 13, "r_gain");
    reg->gr_gain = common_check_bits(reg->gr_gain, 13, "gr_gain");
    reg->gb_gain = common_check_bits(reg->gb_gain, 13, "gb_gain");
    reg->b_gain = common_check_bits(reg->b_gain, 13, "b_gain");


    spdlog::info("================= awb gain reg=================");
    spdlog::info("bypass {}", reg->bypass);
    spdlog::info("r_gain {}", reg->r_gain);
    spdlog::info("gr_gain {}", reg->gr_gain);
    spdlog::info("gb_gain {}", reg->gb_gain);
    spdlog::info("b_gain {}", reg->b_gain);
    spdlog::info("================= awb gain reg=================");
}