#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gtm_stat_reg_s
{
    uint32_t bypass;
    uint32_t rgb2y[3];
    uint32_t w_ratio;
    uint32_t h_ratio;
}gtm_stat_reg_t;

class gtm_stat :public hw_base {
public:
    gtm_stat() = delete;
    gtm_stat(const gtm_stat& cp) = delete;
    gtm_stat(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~gtm_stat();

private:
    void checkparameters(gtm_stat_reg_t* reg);
    gtm_stat_reg_t* gtm_stat_reg;
    uint32_t bypass;
    std::vector<uint32_t> rgb2y;
    uint32_t w_ratio;
    uint32_t h_ratio;
};