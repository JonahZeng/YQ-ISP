#pragma once
#include "hw_base.h"
#include <stdint.h>

class yuv_decode_hw :public hw_base {
public:
    yuv_decode_hw() = delete;
    yuv_decode_hw(const yuv_decode_hw& cp) = delete;
    yuv_decode_hw& operator=(const yuv_decode_hw& cp) = delete;
    yuv_decode_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~yuv_decode_hw();
private:
    uint32_t bypass;
    char jpg_file_name[256];
    uint32_t do_fancy_upsampling;
};
