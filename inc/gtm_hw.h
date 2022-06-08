#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gtm_reg_s
{
    uint32_t bypass;
    uint32_t rgb2y[3];
    uint32_t gain_lut[257];
}gtm_reg_t;

class gtm_hw :public hw_base {
public:
    gtm_hw() = delete;
    gtm_hw(const gtm_hw& cp) = delete;
    gtm_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~gtm_hw();

private:
    void checkparameters(gtm_reg_t* reg);
    gtm_reg_t* gtm_reg;
    uint32_t bypass;
    std::vector<uint32_t> rgb2y;
    std::vector<uint32_t> gain_lut;
};