#pragma once

#include "blc.h"
#include "lsc.h"
#include "ae_stat.h"
#include "awb_gain.h"
#include "demosaic.h"
#include "cc.h"
#include "gtm.h"
#include "gamma.h"
#include "gtm_stat.h"
#include "rgb2yuv.h"
#include "sensor_crop.h"
#include "yuv422_conv.h"
#include "cac_rgb.h"
#include "yuv2rgb_8bit.h"
#include "hsv_lut.h"
#include "prophoto2srgb.h"

typedef struct fe_module_reg_s {
    blc_reg_t blc_reg;
    lsc_reg_t lsc_reg;
    ae_stat_reg_t ae_stat_reg;
    awbgain_reg_t awbgain_reg;
    demosaic_reg_t demosaic_reg;
    cc_reg_t cc_reg;
    gtm_reg_t gtm_reg;
    gamma_reg_t gamma_reg;
    gtm_stat_reg_t gtm_stat_reg;
    rgb2yuv_reg_t rgb2yuv_reg;
    sensor_crop_reg_t sensor_crop_reg;
    yuv422_conv_reg_t yuv422_conv_reg;
    cac_rgb_reg_t cac_rgb_reg;
    yuv2rgb_8b_reg_t yuv2rgb_8b_reg;
    hsv_lut_reg_t hsv_lut_reg;
    prophoto2srgb_reg_t prophoto2srgb_reg;
}fe_module_reg_t;

class fe_firmware :public hw_base {
public:
    fe_firmware() = delete;
    fe_firmware(const fe_firmware& cp) = delete;
    fe_firmware(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~fe_firmware();
private:
    uint32_t bypass;
};
