#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct gamma_reg_s
{
    uint32_t bypass;
    uint32_t gamma_x1024;
}gamma_reg_t;

class gamma :public hw_base {
public:
    gamma() = delete;
    gamma(const gamma& cp) = delete;
    gamma(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~gamma();

private:
    void checkparameters(gamma_reg_t* reg);
    uint32_t bypass;
    uint32_t gamma_x1024;
};
