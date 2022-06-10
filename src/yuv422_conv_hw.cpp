#include "pipe_register.h"
#include <assert.h>

yuv422_conv_hw::yuv422_conv_hw(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    filter_coeff.resize(4);
    filter_coeff[0] = 32;
    filter_coeff[1] = 96;
    filter_coeff[2] = 96;
    filter_coeff[0] = 32;

    yuv422_conv_reg = new yuv422_conv_reg_t;
}

void yuv422_conv_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_y = in[0];
    data_buffer* input_u = in[1];
    data_buffer* input_v = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t*fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(yuv422_conv_reg, &fe_reg->yuv422_conv_reg, sizeof(yuv422_conv_reg_t));
    }

    if (xmlConfigValid)
    {
        for (int32_t i = 0; i < 4; i++)
        {
            yuv422_conv_reg->filter_coeff[i] = filter_coeff[i];
        }
    }

    checkparameters(yuv422_conv_reg);

    uint32_t full_xsize = input_y->width;
    uint32_t full_ysize = input_y->height;

    uint32_t cb_xsize = input_u->width;
    uint32_t cb_ysize = input_u->height;

    uint32_t cr_xsize = input_v->width;
    uint32_t cr_ysize = input_v->height;

    log_debug("y size= %d, %d, u size = %d, %d, v size = %d, %d\n", full_xsize, full_ysize, cb_xsize, cb_ysize, cr_xsize, cr_ysize);

    assert(cb_xsize % 2 == 0);
    assert(cb_xsize == cr_xsize && cb_ysize == cr_ysize);

    data_buffer* output_y = new data_buffer(full_xsize, full_ysize, input_y->data_type, BAYER_UNSUPPORT, "yuv422_conv_out_y");
    out[0] = output_y;

    data_buffer* output_u = new data_buffer(cb_xsize/2, cb_ysize, input_u->data_type, BAYER_UNSUPPORT, "yuv422_conv_out_u");
    out[1] = output_u;

    data_buffer* output_v = new data_buffer(cr_xsize/2, cr_ysize, input_v->data_type, BAYER_UNSUPPORT, "yuv422_conv_out_v");
    out[2] = output_v;


    for (uint32_t sz = 0; sz < full_xsize*full_ysize; sz++)
    {
        output_y->data_ptr[sz] = input_y->data_ptr[sz];
    }

    int32_t idx0, idx1, idx2, idx3;
    for (uint32_t row = 0; row < cb_ysize; row++)
    {
        for (uint32_t col = 0; col < cb_xsize / 2; col++)
        {
            idx0 = (int32_t)col * 2 - 1;
            idx1 = (int32_t)col * 2;
            idx2 = (int32_t)col * 2 + 1;
            idx3 = (int32_t)col * 2 + 2;
            if (idx0 < 0)
                idx0 = 0;
            if (idx3 > int32_t(cb_xsize - 1))
                idx3 = int32_t(cb_xsize - 1);
            uint16_t u0 = (input_u->data_ptr[row*cb_xsize + idx0]) >> (16 - 10);
            uint16_t u1 = (input_u->data_ptr[row*cb_xsize + idx1]) >> (16 - 10);
            uint16_t u2 = (input_u->data_ptr[row*cb_xsize + idx2]) >> (16 - 10);
            uint16_t u3 = (input_u->data_ptr[row*cb_xsize + idx3]) >> (16 - 10);
            output_u->data_ptr[row*(cb_xsize / 2) + col] = 
                (u0 * yuv422_conv_reg->filter_coeff[0] + u1 * yuv422_conv_reg->filter_coeff[1] + \
                u2 * yuv422_conv_reg->filter_coeff[2] + u3 * yuv422_conv_reg->filter_coeff[3]) / 16;
            uint16_t v0 = (input_v->data_ptr[row*cb_xsize + idx0]) >> (16 - 10);
            uint16_t v1 = (input_v->data_ptr[row*cb_xsize + idx1]) >> (16 - 10);
            uint16_t v2 = (input_v->data_ptr[row*cb_xsize + idx2]) >> (16 - 10);
            uint16_t v3 = (input_v->data_ptr[row*cb_xsize + idx3]) >> (16 - 10);
            output_v->data_ptr[row*(cb_xsize / 2) + col] =
                (v0 * yuv422_conv_reg->filter_coeff[0] + v1 * yuv422_conv_reg->filter_coeff[1] + \
                    v2 * yuv422_conv_reg->filter_coeff[2] + v3 * yuv422_conv_reg->filter_coeff[3]) / 16;
            
        }
    }

    for (uint32_t row = 0; row < cb_ysize; row++)
    {
        for (uint32_t col = 0; col < cb_xsize / 2; col++)
        {
            output_u->data_ptr[row*(cb_xsize / 2) + col] = (output_u->data_ptr[row*(cb_xsize / 2) + col]) << (16 - 10);
            output_v->data_ptr[row*(cb_xsize / 2) + col] = (output_v->data_ptr[row*(cb_xsize / 2) + col]) << (16 - 10);
        }
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void yuv422_conv_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"filter_coeff",        VECT_UINT32,      &this->filter_coeff,          4}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

yuv422_conv_hw::~yuv422_conv_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (yuv422_conv_reg != nullptr)
    {
        delete yuv422_conv_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void yuv422_conv_hw::checkparameters(yuv422_conv_reg_t* reg)
{
    log_info("================= yuv422_conv reg=================\n");
    log_array("filter_coeff:\n", "%4d, ", reg->filter_coeff, 4, 4);
    log_info("================= yuv422_conv reg=================\n");
}

