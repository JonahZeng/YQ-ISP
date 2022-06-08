﻿#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct yuv422_conv_reg_s
{
    uint32_t filter_coeff[4];
}yuv422_conv_reg_t;

class yuv422_conv :public hw_base {
public:
    yuv422_conv() = delete;
    yuv422_conv(const yuv422_conv& cp) = delete;
    yuv422_conv(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~yuv422_conv();

private:
    void checkparameters(yuv422_conv_reg_t* reg);
    std::vector<uint32_t> filter_coeff;
    yuv422_conv_reg_t* yuv422_conv_reg;
};
