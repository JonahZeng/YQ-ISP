#pragma once
#include "hw_base.h"
#include <stdint.h>

class yuv_decode :public hw_base {
public:
    yuv_decode() = delete;
    yuv_decode(const yuv_decode& cp) = delete;
    yuv_decode& operator=(const yuv_decode& cp) = delete;
    yuv_decode(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~yuv_decode();
private:
    uint32_t bypass;
    char jpg_file_name[256];
    uint32_t do_fancy_upsampling;
};
