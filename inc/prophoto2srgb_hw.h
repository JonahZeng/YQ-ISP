#pragma once
#include "hw_base.h"
#include <stdint.h>

typedef struct prophoto2srgb_reg_s
{
    uint32_t bypass;
    uint32_t hlr_en;
    int32_t ccm[9];
}prophoto2srgb_reg_t;

class prophoto2srgb_hw :public hw_base {
public:
    prophoto2srgb_hw() = delete;
    prophoto2srgb_hw(const prophoto2srgb_hw& cp) = delete;
    prophoto2srgb_hw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~prophoto2srgb_hw();

private:
    void checkparameters(prophoto2srgb_reg_t* reg);
    prophoto2srgb_reg_t* prophoto2srgb_reg;
    uint32_t bypass;
    uint32_t hlr_en;
    std::vector<int32_t> ccm;
};
