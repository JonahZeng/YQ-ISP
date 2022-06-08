#pragma once

#include "hw_base.h"
#include <stdint.h>

#define LSC_GRID_COLS 41
#define LSC_GRID_ROWS 31

typedef struct lsc_reg_s
{
    uint32_t bypass;
    uint32_t block_size_x;
    uint32_t block_size_y;

    uint32_t block_start_x_idx;
    uint32_t block_start_x_oft;

    uint32_t block_start_y_idx;
    uint32_t block_start_y_oft;

    uint32_t luma_gain[LSC_GRID_ROWS * LSC_GRID_COLS];
}lsc_reg_t;

class lsc_hw :public hw_base {
public:
    lsc_hw() = delete;
    lsc_hw(const lsc_hw& cp) = delete;
    lsc_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~lsc_hw();

private:
    void checkparameters(lsc_reg_t* reg);
    lsc_reg_t* lsc_reg;
    uint32_t bypass;
    uint32_t block_size_x;
    uint32_t block_size_y;

    uint32_t block_start_x_idx;
    uint32_t block_start_x_oft;

    uint32_t block_start_y_idx;
    uint32_t block_start_y_oft;

    std::vector<uint32_t> luma_gain;
};
