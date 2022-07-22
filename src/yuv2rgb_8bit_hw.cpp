#include "pipe_register.h"
#include <assert.h>

yuv2rgb_8b_hw::yuv2rgb_8b_hw(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name), yuv2rgb_coeff(9, 0)
{
    bypass = 0;
    yuv2rgb_8b_reg = new yuv2rgb_8b_reg_t;
}

void yuv2rgb_8b_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_y = in[0];
    data_buffer* input_u = in[1];
    data_buffer* input_v = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(yuv2rgb_8b_reg, &fe_reg->yuv2rgb_8b_reg, sizeof(yuv2rgb_8b_reg_t));
    }

    yuv2rgb_8b_reg->bypass = bypass;

    if (xmlConfigValid)
    {
        for (int32_t i = 0; i < 9; i++)
        {
            yuv2rgb_8b_reg->yuv2rgb_coeff[i] = yuv2rgb_coeff[i];
        }
    }

    checkparameters(yuv2rgb_8b_reg);

    uint32_t xsize = input_y->width;
    uint32_t ysize = input_y->height;

    assert(xsize == input_u->width && ysize == input_u->height);

    data_buffer* output_r = new data_buffer(xsize, ysize, DATA_TYPE_R, BAYER_UNSUPPORT, "yuv2rgb_out_r");
    out[0] = output_r;

    data_buffer* output_g = new data_buffer(xsize, ysize, DATA_TYPE_G, BAYER_UNSUPPORT, "yuv2rgb_out_g");
    out[1] = output_g;

    data_buffer* output_b = new data_buffer(xsize, ysize, DATA_TYPE_B, BAYER_UNSUPPORT, "yuv2rgb_out_b");
    out[2] = output_b;


    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        int32_t y = (input_y->data_ptr[sz]) >> (16 - 8);
        int32_t u = (input_u->data_ptr[sz]) >> (16 - 8);
        int32_t v = (input_v->data_ptr[sz]) >> (16 - 8);
        u = u - 128;
        v = v - 128;

        int32_t r = (y * yuv2rgb_8b_reg->yuv2rgb_coeff[0] + u * yuv2rgb_8b_reg->yuv2rgb_coeff[1] + v * yuv2rgb_8b_reg->yuv2rgb_coeff[2] + 512) >> 10;
        int32_t g = (y * yuv2rgb_8b_reg->yuv2rgb_coeff[3] + u * yuv2rgb_8b_reg->yuv2rgb_coeff[4] + v * yuv2rgb_8b_reg->yuv2rgb_coeff[5] + 512) >> 10;
        int32_t b = (y * yuv2rgb_8b_reg->yuv2rgb_coeff[6] + u * yuv2rgb_8b_reg->yuv2rgb_coeff[7] + v * yuv2rgb_8b_reg->yuv2rgb_coeff[8] + 512) >> 10;

        r = (r > 255) ? 255 : (r < 0 ? 0 : r);
        g = (g > 255) ? 255 : (g < 0 ? 0 : g);
        b = (b > 255) ? 255 : (b < 0 ? 0 : b);

        output_r->data_ptr[sz] = (uint16_t)r;
        output_g->data_ptr[sz] = (uint16_t)g;
        output_b->data_ptr[sz] = (uint16_t)b;
    }


    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        output_r->data_ptr[sz] = (output_r->data_ptr[sz]) << (16 - 8);
        output_g->data_ptr[sz] = (output_g->data_ptr[sz]) << (16 - 8);
        output_b->data_ptr[sz] = (output_b->data_ptr[sz]) << (16 - 8);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void yuv2rgb_8b_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,      &this->bypass                 },
        {"yuv2rgb_coeff",          VECT_INT32,   &this->yuv2rgb_coeff,        9}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

yuv2rgb_8b_hw::~yuv2rgb_8b_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (yuv2rgb_8b_reg != nullptr)
    {
        delete yuv2rgb_8b_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void yuv2rgb_8b_hw::checkparameters(yuv2rgb_8b_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (int32_t i = 0; i < 9; i++)
    {
        reg->yuv2rgb_coeff[i] = common_check_bits_ex(reg->yuv2rgb_coeff[i], 12, "yuv2rgb_coeff");
    }

    log_info("================= yuv2rgb 8b reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_1d_array("yuv2rgb_coeff:\n", "%5d, ", reg->yuv2rgb_coeff, 9, 3);
    log_info("================= yuv2rgb 8b reg=================\n");
}
