#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct rgbir_remosaic_reg_s
{
    uint32_t bypass;
    int32_t ir_subtract_r[17];
    int32_t ir_subtract_g[17];
    int32_t ir_subtract_b[17];
}rgbir_remosaic_reg_t;

class rgbir_remosaic_hw :public hw_base {
public:
    rgbir_remosaic_hw() = delete;
    rgbir_remosaic_hw(const rgbir_remosaic_hw& cp) = delete;
    rgbir_remosaic_hw& operator=(rgbir_remosaic_hw& cp) = delete;
    rgbir_remosaic_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~rgbir_remosaic_hw();

private:
    void checkparameters(rgbir_remosaic_reg_t* reg);
    rgbir_remosaic_reg_t* rgbir_remosaic_reg;
    uint32_t bypass;
    std::vector<int32_t> ir_subtract_r;
    std::vector<int32_t> ir_subtract_g;
    std::vector<int32_t> ir_subtract_b;
};
