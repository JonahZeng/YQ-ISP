#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct demosaic_reg_s
{
    uint32_t bypass;
}demosaic_reg_t;

class demosaic_hw :public hw_base {
public:
    demosaic_hw() = delete;
    demosaic_hw(const demosaic_hw& cp) = delete;
    demosaic_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~demosaic_hw();

private:
    void checkparameters(demosaic_reg_t* reg);
    demosaic_reg_t* demosaic_reg;
    uint32_t bypass;
};
