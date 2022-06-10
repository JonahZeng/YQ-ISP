#pragma once

#include "blc_hw.h"
#include "lsc_hw.h"
#include "ae_stat_hw.h"
#include "awb_gain_hw.h"
#include "demosaic_hw.h"
#include "cc_hw.h"
#include "gtm_hw.h"
#include "gamma_hw.h"
#include "gtm_stat_hw.h"
#include "rgb2yuv_hw.h"
#include "raw_crop_hw.h"
#include "yuv422_conv_hw.h"
#include "cac_rgb_hw.h"
#include "yuv2rgb_8bit_hw.h"
#include "hsv_lut_hw.h"
#include "prophoto2srgb_hw.h"

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
    raw_crop_reg_t raw_crop_reg;
    yuv422_conv_reg_t yuv422_conv_reg;
    cac_rgb_reg_t cac_rgb_reg;
    yuv2rgb_8b_reg_t yuv2rgb_8b_reg;
    hsv_lut_reg_t hsv_lut_reg;
    prophoto2srgb_reg_t prophoto2srgb_reg;
}fe_module_reg_t;
