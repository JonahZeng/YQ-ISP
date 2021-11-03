#pragma once

#include "hw_base.h"
#include <stdint.h>

class yuv_encode :public hw_base {
public:
    yuv_encode() = delete;
    yuv_encode(const yuv_encode& cp) = delete;
    yuv_encode(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~yuv_encode();
private:
    uint32_t bypass;
};
