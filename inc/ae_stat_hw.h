#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct ae_stat_reg_s
{
    uint32_t bypass;
    uint32_t block_width;
    uint32_t block_height;
    uint32_t skip_step_x;
    uint32_t skip_step_y;
    uint32_t start_x[32];
    uint32_t start_y[32];
}ae_stat_reg_t;

class ae_stat_hw :public hw_base {
public:
    ae_stat_hw() = delete;
    ae_stat_hw(const ae_stat_hw& cp) = delete;
    ae_stat_hw& operator=(const ae_stat_hw& cp) = delete;
    ae_stat_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~ae_stat_hw();

private:
    void checkparameters(ae_stat_reg_t* reg);
    ae_stat_reg_t* ae_stat_reg;
    uint32_t bypass;
    uint32_t block_width;
    uint32_t block_height;
    uint32_t skip_step_x;
    uint32_t skip_step_y;
    std::vector<uint32_t> start_x;
    std::vector<uint32_t> start_y;
};
