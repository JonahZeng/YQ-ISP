#pragma once
#include <stdint.h>

typedef struct ae_dng_md_s
{
    uint32_t reserved;
}ae_dng_md_t;

typedef struct blc_dng_md_s
{
    int32_t r_black_level;
    int32_t gr_black_level;
    int32_t gb_black_level;
    int32_t b_black_level;

    int32_t white_level;
}blc_dng_md_t;

typedef struct dng_md_s
{
    ae_dng_md_t ae_md;
    blc_dng_md_t blc_md;
}dng_md_t;