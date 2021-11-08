#include "fe_firmware.h"

sensor_crop::sensor_crop(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 1;
    origin_x = 0;
    origin_y = 0;
    width = 0;
    height = 0;
}

void sensor_crop::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;
    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
    sensor_crop_reg_t* sensor_crop_reg = &fe_reg->sensor_crop_reg;

    sensor_crop_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        sensor_crop_reg->origin_x = origin_x;
        sensor_crop_reg->origin_y = origin_y;
        sensor_crop_reg->width = width;
        sensor_crop_reg->height = height;
    }

    checkparameters(sensor_crop_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    if (sensor_crop_reg->bypass > 0)
    {
        data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, "sensor_crop_out0");
        uint16_t* out0_ptr = output0->data_ptr;
        out[0] = output0;

        for (uint32_t sz = 0; sz < xsize*ysize; sz++)
        {
            out0_ptr[sz] = input_raw->data_ptr[sz];
        }
    }
    else
    {
        uint32_t new_xsize = sensor_crop_reg->width;
        uint32_t new_ysize = sensor_crop_reg->height;
        data_buffer* output0 = new data_buffer(new_xsize, new_ysize, input_raw->data_type, input_raw->bayer_pattern, "sensor_crop_out0");
        uint16_t* out0_ptr = output0->data_ptr;
        out[0] = output0;

        for (uint32_t row = 0; row < new_ysize; row++)
        {
            for (uint32_t col = 0; col < new_xsize; col++)
            {
                out0_ptr[row*new_xsize + col] = input_raw->data_ptr[(row + sensor_crop_reg->origin_y)*xsize + col + sensor_crop_reg->origin_x];
            }
        }
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void sensor_crop::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          },
        {"origin_x",               UINT_32,     &this->origin_x        },
        {"origin_y",               UINT_32,     &this->origin_y        },
        {"width",                  UINT_32,     &this->width           },
        {"height",                 UINT_32,     &this->height          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s init run end\n", name);
}

sensor_crop::~sensor_crop()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    log_info("%s module deinit end\n", __FUNCTION__);
}


void sensor_crop::checkparameters(sensor_crop_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->origin_x = common_check_bits(reg->origin_x, 13, "origin_x");
    reg->origin_y = common_check_bits(reg->origin_y, 13, "origin_y");
    reg->width = common_check_bits(reg->width, 13, "width");
    reg->height = common_check_bits(reg->height, 13, "height");

    log_info("================= sensor_crop reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("origin_x %d\n", reg->origin_x);
    log_info("origin_y %d\n", reg->origin_y);
    log_info("width %d\n", reg->width);
    log_info("height %d\n", reg->height);
    log_info("================= sensor_crop reg=================\n");
}