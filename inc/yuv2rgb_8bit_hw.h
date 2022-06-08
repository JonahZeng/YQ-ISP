#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct yuv2rgb_8b_reg_s
{
    uint32_t bypass;
    int32_t yuv2rgb_coeff[9];
}yuv2rgb_8b_reg_t;

class yuv2rgb_8b_hw :public hw_base {
public:
    yuv2rgb_8b_hw() = delete;
    yuv2rgb_8b_hw(const yuv2rgb_8b_hw& cp) = delete;
    yuv2rgb_8b_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~yuv2rgb_8b_hw();

private:
    void checkparameters(yuv2rgb_8b_reg_t* reg);
    yuv2rgb_8b_reg_t* yuv2rgb_8b_reg;
    uint32_t bypass;
    std::vector<int32_t> yuv2rgb_coeff;
};
