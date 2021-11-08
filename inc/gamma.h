#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gamma_reg_s
{
    uint32_t bypass;
    uint32_t gamma_lut[257];
}gamma_reg_t;

class Gamma :public hw_base {
public:
    Gamma() = delete;
    Gamma(const Gamma& cp) = delete;
    Gamma(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~Gamma();

private:
    void checkparameters(gamma_reg_t* reg);
    uint32_t bypass;
    vector<uint32_t> gamma_lut;
};
