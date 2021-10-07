#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct demosaic_reg_s
{
    uint32_t bypass;
}demosaic_reg_t;

class demosaic :public hw_base {
public:
    demosaic() = delete;
    demosaic(const demosaic& cp) = delete;
    demosaic(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~demosaic();

private:
    void checkparameters(demosaic_reg_t* reg);
    uint32_t bypass;
};
