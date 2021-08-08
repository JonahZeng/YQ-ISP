#pragma once

#include <stdint.h>

typedef struct awb_stat_info_s {
    uint32_t r_sum[32 * 32];
    uint32_t g_sum[32 * 32];
    uint32_t b_sum[32 * 32];
    uint32_t r_cnt[32 * 32];
    uint32_t g_cnt[32 * 32];
    uint32_t b_cnt[32 * 32];
}awb_stat_info_t;

typedef struct drc_stat_info_s {
    uint32_t reserved;
}drc_stat_info_t;

typedef struct shading_stat_info_s {
    uint32_t reserved;
}shading_stat_info_t;

typedef struct ae_stat_info_s {
    uint32_t reserved;
}ae_stat_info_t;

typedef struct af_stat_info_s {
    uint32_t reserved;
}af_stat_info_t;

typedef struct ce_stat_info_s {
    uint32_t reserved;
}ce_stat_info_t;

typedef struct statistic_info_s {
    awb_stat_info_t awb_stat;
    ae_stat_info_t ae_stat;
    af_stat_info_t af_stat;
    drc_stat_info_t drc_stat;
    shading_stat_info_t shading_stat;
    ce_stat_info_t ce_stat;
}statistic_info_t;