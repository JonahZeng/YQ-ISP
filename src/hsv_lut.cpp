#include "fe_firmware.h"
#include <assert.h>

hsv_lut::hsv_lut(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name),
    bypass(0), hsv_lut_reg(new hsv_lut_reg_t)
{
}

static void hsv_lut_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const hsv_lut_reg_t* hsv_lut_reg)
{
    int32_t r_in, g_in, b_in;
    int32_t r_out, g_out, b_out;
    //for (uint32_t row = 0; row < ysize; row++)
    //{
    //    for (uint32_t col = 0; col < xsize; col++)
    //    {
    //        r_in = r[row * xsize + col];
    //        g_in = g[row * xsize + col];
    //        b_in = b[row * xsize + col];
    //        
    //    }
    //}
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