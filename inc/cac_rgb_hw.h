#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct cac_rgb_reg_s
{
    uint32_t bypass;
}cac_rgb_reg_t;

class cac_rgb_hw :public hw_base {
public:
    cac_rgb_hw() = delete;
    cac_rgb_hw(const cac_rgb_hw& cp) = delete;
    cac_rgb_hw& operator=(const cac_rgb_hw& cp) = delete;
    cac_rgb_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~cac_rgb_hw();

private:
    void checkparameters(cac_rgb_reg_t* reg);
    cac_rgb_reg_t* cac_rgb_reg;
    uint32_t bypass;
};