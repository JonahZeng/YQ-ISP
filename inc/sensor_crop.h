#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct sensor_crop_reg_s
{
    uint32_t bypass;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
}sensor_crop_reg_t;

class sensor_crop :public hw_base {
public:
    sensor_crop() = delete;
    sensor_crop(const sensor_crop& cp) = delete;
    sensor_crop(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~sensor_crop();

private:
    void checkparameters(sensor_crop_reg_t* reg);
    sensor_crop_reg_t* sensor_crop_reg;
    uint32_t bypass;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
};
