#include "fe_firmware.h"

rgb2yuv::rgb2yuv(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name), rgb2yuv_coeff(9, 0)
{
    bypass = 0;
}

void rgb2yuv::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_r = in[0];
    data_buffer* input_g = in[1];
    data_buffer* input_b = in[2];

    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
    rgb2yuv_reg_t* rgb2yuv_reg = &fe_reg->rgb2yuv_reg;

    rgb2yuv_reg->bypass = bypass;

    if (xmlConfigValid)
    {
        for (int32_t i = 0; i < 9; i++)
        {
            rgb2yuv_reg->rgb2yuv_coeff[i] = rgb2yuv_coeff[i];
        }
    }

    checkparameters(rgb2yuv_reg);

    uint32_t xsize = input_r->width;
    uint32_t ysize = input_r->height;

    data_buffer* output_y = new data_buffer(xsize, ysize, DATA_TYPE_Y, BAYER_UNSUPPORT, "rgb2yuv_out_y");
    out[0] = output_y;

    data_buffer* output_u = new data_buffer(xsize, ysize, DATA_TYPE_U, BAYER_UNSUPPORT, "rgb2yuv_out_u");
    out[1] = output_u;

    data_buffer* output_v = new data_buffer(xsize, ysize, DATA_TYPE_V, BAYER_UNSUPPORT, "rgb2yuv_out_v");
    out[2] = output_v;


    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        uint16_t r = (input_r->data_ptr[sz]) >> (16 - 10);
        uint16_t g = (input_g->data_ptr[sz]) >> (16 - 10);
        uint16_t b = (input_b->data_ptr[sz]) >> (16 - 10);

        int32_t y = (r * rgb2yuv_reg->rgb2yuv_coeff[0] + g * rgb2yuv_reg->rgb2yuv_coeff[1] + b * rgb2yuv_reg->rgb2yuv_coeff[2] + 512) >> 10;
        int32_t u = (r * rgb2yuv_reg->rgb2yuv_coeff[3] + g * rgb2yuv_reg->rgb2yuv_coeff[4] + b * rgb2yuv_reg->rgb2yuv_coeff[5] + 512) >> 10;
        int32_t v = (r * rgb2yuv_reg->rgb2yuv_coeff[6] + g * rgb2yuv_reg->rgb2yuv_coeff[7] + b * rgb2yuv_reg->rgb2yuv_coeff[8] + 512) >> 10;
        u = u + 512;
        v = v + 512;
        y = (y > 1023) ? 1023 : (y < 0 ? 0 : y);
        u = (u > 1023) ? 1023 : (u < 0 ? 0 : u);
        v = (v > 1023) ? 1023 : (v < 0 ? 0 : v);

        output_y->data_ptr[sz] = (uint16_t)y;
        output_u->data_ptr[sz] = (uint16_t)u;
        output_v->data_ptr[sz] = (uint16_t)v;
    }


    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        output_y->data_ptr[sz] = (output_y->data_ptr[sz]) << (16 - 10);
        output_u->data_ptr[sz] = (output_u->data_ptr[sz]) << (16 - 10);
        output_v->data_ptr[sz] = (output_v->data_ptr[sz]) << (16 - 10);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void rgb2yuv::init()
{
    log_info("%s run start\n", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,      &this->bypass                 },
        {"rgb2yuv_coeff",          VECT_INT32,   &this->rgb2yuv_coeff,        9}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s run end\n", __FUNCTION__);
}

rgb2yuv::~rgb2yuv()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    log_info("%s module deinit end\n", __FUNCTION__);
}

void rgb2yuv::checkparameters(rgb2yuv_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (int32_t i = 0; i < 9; i++)
    {
        reg->rgb2yuv_coeff[i] = common_check_bits_ex(reg->rgb2yuv_coeff[i], 11, "rgb2yuv_coeff");
    }

    log_info("================= rgb2yuv reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_array("rgb2yuv_coeff:\n", "%5d, ", reg->rgb2yuv_coeff, 9, 3);
    log_info("================= rgb2yuv reg=================\n");
}
