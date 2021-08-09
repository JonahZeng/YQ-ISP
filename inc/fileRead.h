#pragma once

#include "hw_base.h"
#include <stdint.h>

class fileRead :public hw_base {
public:
    fileRead()=delete;
    fileRead(uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~fileRead();
private:
    bayer_type_t bayer;
    uint32_t bit_depth;
    file_type_t file_type;
    char file_name[256];
    char file_type_string[32];
    char bayer_string[32];
};