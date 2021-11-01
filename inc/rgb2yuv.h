#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct rgb2yuv_reg_s
{
    uint32_t bypass;
    int32_t rgb2yuv_coeff[9];
}rgb2yuv_reg_t;

class rgb2yuv :public hw_base {
public:
    rgb2yuv() = delete;
    rgb2yuv(const rgb2yuv& cp) = delete;
    rgb2yuv(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~rgb2yuv();

private:
    void checkparameters(rgb2yuv_reg_t* reg);
    uint32_t bypass;
    vector<int32_t> rgb2yuv_coeff;
};
