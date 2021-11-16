#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct yuv2rgb_8b_reg_s
{
    uint32_t bypass;
    int32_t yuv2rgb_coeff[9];
}yuv2rgb_8b_reg_t;

class yuv2rgb_8b :public hw_base {
public:
    yuv2rgb_8b() = delete;
    yuv2rgb_8b(const yuv2rgb_8b& cp) = delete;
    yuv2rgb_8b(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~yuv2rgb_8b();

private:
    void checkparameters(yuv2rgb_8b_reg_t* reg);
    yuv2rgb_8b_reg_t* yuv2rgb_8b_reg;
    uint32_t bypass;
    vector<int32_t> yuv2rgb_coeff;
};
