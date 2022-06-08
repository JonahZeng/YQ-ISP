#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct raw_crop_reg_s
{
    uint32_t bypass;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
}raw_crop_reg_t;

class raw_crop_hw :public hw_base {
public:
    raw_crop_hw() = delete;
    raw_crop_hw(const raw_crop_hw& cp) = delete;
    raw_crop_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~raw_crop_hw();

private:
    void checkparameters(raw_crop_reg_t* reg);
    raw_crop_reg_t* raw_crop_reg;
    uint32_t bypass;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
};
