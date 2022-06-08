#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct rgb2yuv_reg_s
{
    uint32_t bypass;
    int32_t rgb2yuv_coeff[9];
}rgb2yuv_reg_t;

class rgb2yuv_hw :public hw_base {
public:
    rgb2yuv_hw() = delete;
    rgb2yuv_hw(const rgb2yuv_hw& cp) = delete;
    rgb2yuv_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~rgb2yuv_hw();

private:
    void checkparameters(rgb2yuv_reg_t* reg);
    rgb2yuv_reg_t* rgb2yuv_reg;
    uint32_t bypass;
    std::vector<int32_t> rgb2yuv_coeff;
};
