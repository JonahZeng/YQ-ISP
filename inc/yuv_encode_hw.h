#pragma once

#include "hw_base.h"
#include <stdint.h>

class yuv_encode_hw :public hw_base {
public:
    yuv_encode_hw() = delete;
    yuv_encode_hw(const yuv_encode_hw& cp) = delete;
    yuv_encode_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~yuv_encode_hw();
private:
    uint32_t bypass;
    uint32_t use_input_file_name;
    char output_jpg_file_name[256];
};
