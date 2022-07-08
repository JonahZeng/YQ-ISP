#pragma once

#include "hw_base.h"
#include <stdint.h>

class file_read :public hw_base {
public:
    file_read()=delete;
    file_read(uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~file_read();
private:
    bayer_type_t bayer;
    uint32_t bit_depth;
    file_type_t file_type;
    uint32_t img_width;
    uint32_t img_height;
    char file_name[256];
    char file_type_string[32];
    char bayer_string[32];
};