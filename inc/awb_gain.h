#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct awbgain_reg_s
{
    uint32_t bypass;
    uint32_t r_gain;
    uint32_t gr_gain;
    uint32_t gb_gain;
    uint32_t b_gain;

    uint32_t ae_compensat_gain;
}awbgain_reg_t;

class awbgain :public hw_base {
public:
    awbgain() = delete;
    awbgain(const awbgain& cp) = delete;
    awbgain(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~awbgain();

private:
    void checkparameters(awbgain_reg_t* reg);
    awbgain_reg_t* awbgain_reg;
    uint32_t bypass;
    uint32_t r_gain;
    uint32_t gr_gain;
    uint32_t gb_gain;
    uint32_t b_gain;

    uint32_t ae_compensat_gain;
};

