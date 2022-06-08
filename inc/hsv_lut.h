#pragma once

#include "hw_base.h"
#include <stdint.h>
#include "dng_hue_sat_map.h"

typedef struct hvs_lut_reg_s
{
    uint32_t bypass;
    uint32_t twoD_enable;
    uint32_t threeD_enable;
    uint32_t twoD_map_encoding;
    uint32_t threeD_map_encoding;

    uint32_t twoDHueDivisions;
    uint32_t twoDSatDivisions;

    dng_hue_sat_map::HSBModify twoDmap[90 * 30 * 30];
    uint32_t threeDHueDivisions;
    uint32_t threeDSatDivisions;
    uint32_t threeDValDivisions;

    dng_hue_sat_map::HSBModify threeDmap[90 * 30 * 30];
}hsv_lut_reg_t;

class hsv_lut :public hw_base {
public:
    hsv_lut() = delete;
    hsv_lut(const hsv_lut& cp) = delete;
    hsv_lut(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~hsv_lut();

private:
    void checkparameters(hsv_lut_reg_t* reg);
    hsv_lut_reg_t* hsv_lut_reg;
    uint32_t bypass;
};
