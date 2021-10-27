#pragma once
#include <stdint.h>

typedef struct cc_wp_cc_group_s
{
    uint32_t wp_r;
    uint32_t wp_b;
    int32_t ccm[9];
}cc_wp_cc_group_t;

typedef struct chromatix_cc_s
{
    uint32_t cc_mode; // 1: dng metadata, 2: calib
    cc_wp_cc_group_t calib_info[3]; //D75 D50 D28
}chromatix_cc_t;
