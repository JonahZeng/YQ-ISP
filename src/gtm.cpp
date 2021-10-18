#include "fe_firmware.h"
#include <sstream>

gtm::gtm(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    tone_curve.resize(257);
}

static void gtm_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const gtm_reg_t* gtm_reg)
{
    const uint32_t* rgb2y = gtm_reg->rgb2y;
    const uint32_t* tone_curve = gtm_reg->tone_curve;
    uint32_t y_tmp = 0;
    uint32_t dst_y;
    double ratio;
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            y_tmp = r[row * xsize + col] * rgb2y[0] + g[row * xsize + col] * rgb2y[1] + b[row * xsize + col] * rgb2y[2];
            y_tmp = (y_tmp + 512) >> 10;
            //spdlog::info("{}, {}, {}, y_tmp {}", r[row * xsize + col], g[row * xsize + col], b[row * xsize + col], y_tmp);
            if (y_tmp > (1U << 14) - 1)
                y_tmp = (1U << 14) - 1;
            uint32_t idx1 = y_tmp >> 6;
            uint32_t idx2 = idx1 + 1;
            if (idx2 > 256)
                idx2 = 256;
            uint32_t delta = y_tmp - idx1 * 64;
            dst_y = tone_curve[idx1] + delta * (tone_curve[idx2] - tone_curve[idx1]) / 64;
            //spdlog::info("dst_y {}", dst_y);
            if (y_tmp == 0)
                ratio = 0.0;
            else
                ratio = (double)dst_y / (double)y_tmp;

            uint32_t b_out = uint32_t(b[row * xsize + col] * ratio);
            if (b_out > (1U << 14) - 1U)
                b_out = (1U << 14) - 1U;
            out_b[row * xsize + col] = uint16_t(b_out);
            uint32_t g_out = uint32_t(g[row * xsize + col] * ratio);
            if (g_out > (1U << 14) - 1U)
                g_out = (1U << 14) - 1U;
            out_g[row * xsize + col] = uint16_t(g_out);
            uint32_t r_out = uint32_t(r[row * xsize + col] * ratio);
            if (r_out > (1U << 14) - 1U)
                r_out = (1U << 14) - 1U;
            out_r[row * xsize + col] = uint16_t(r_out);
        }
    }
}

void gtm::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];
    
    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
    gtm_reg_t* gtm_reg = &fe_reg->gtm_reg;

    gtm_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        gtm_reg->rgb2y[0] = rgb2y[0];
        gtm_reg->rgb2y[1] = rgb2y[1];
        gtm_reg->rgb2y[2] = rgb2y[2];

        for (int32_t i = 0; i < 257; i++)
        {
            gtm_reg->tone_curve[i] = tone_curve[i];
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

    delete[] tmp0;
    delete[] tmp1;
    delete[] tmp2;

    hw_base::hw_run(stat_out, frame_cnt);
    spdlog::info("{0} run end", __FUNCTION__);
}

void gtm::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,         &this->bypass          },
        {"rgb2y",                  VECT_UINT32,     &this->rgb2y,         3},
        {"tone_curve",             VECT_UINT32,     &this->tone_curve,  257}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}

gtm::~gtm()
{
    spdlog::info("{0} module deinit start", __FUNCTION__);
    spdlog::info("{0} module deinit end", __FUNCTION__);
}

void gtm::checkparameters(gtm_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (int32_t i = 0; i < 3; i++)
    {
        reg->rgb2y[i] = common_check_bits(reg->rgb2y[i], 10, "rgb2y");
    }
    for (int32_t i = 0; i < 257; i++)
    {
        reg->tone_curve[i] = common_check_bits(reg->tone_curve[i], 14, "tone_curve");
    }

    spdlog::info("================= gtm reg=================");
    spdlog::info("bypass {}", reg->bypass);
    spdlog::info("rgb2y {} {} {}", reg->rgb2y[0], reg->rgb2y[1], reg->rgb2y[2]);
    std::stringstream ostr;

    for (int32_t i = 0; i < 257; i++)
    {
        ostr << reg->tone_curve[i] << ", ";
        if (i % 32 == 31)
        {
            ostr << std::endl;
        }
    }

    spdlog::info("tone_curve \n{}", ostr.str());
    spdlog::info("================= gtm reg=================");
}