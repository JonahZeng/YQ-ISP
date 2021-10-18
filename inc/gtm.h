#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gtm_reg_s
{
    uint32_t bypass;
    uint32_t rgb2y[3];
    uint32_t tone_curve[257];
}gtm_reg_t;

class gtm :public hw_base {
public:
    gtm() = delete;
    gtm(const gtm& cp) = delete;
    gtm(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~gtm();

private:
    void checkparameters(gtm_reg_t* reg);
    uint32_t bypass;
    vector<uint32_t> rgb2y;
    vector<uint32_t> tone_curve;
};