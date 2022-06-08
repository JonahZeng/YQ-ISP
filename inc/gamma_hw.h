#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gamma_reg_s
{
    uint32_t bypass;
    uint32_t gamma_lut[257];
}gamma_reg_t;

class Gamma_hw :public hw_base {
public:
    Gamma_hw() = delete;
    Gamma_hw(const Gamma_hw& cp) = delete;
    Gamma_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~Gamma_hw();

private:
    void checkparameters(gamma_reg_t* reg);
    gamma_reg_t* gamma_reg;
    uint32_t bypass;
    std::vector<uint32_t> gamma_lut;
};
