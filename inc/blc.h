#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct blc_reg_s
{
    uint32_t bypass;
    int32_t blc_r;
    int32_t blc_gr;
    int32_t blc_gb;
    int32_t blc_b;

    uint32_t normalize_en;
    uint32_t white_level;
}blc_reg_t;

class blc :public hw_base {
public:
    blc() = delete;
    blc(const blc& cp) = delete;
    blc(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~blc();
    
private:
    void checkparameters(blc_reg_t* reg);
    blc_reg_t* blc_reg;
    uint32_t bypass;
    int32_t blc_r;
    int32_t blc_gr;
    int32_t blc_gb;
    int32_t blc_b;
    uint32_t normalize_en;
    uint32_t white_level;
};
