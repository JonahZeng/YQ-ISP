#pragma once
#include <stdint.h>
#include <string>

typedef struct ae_dng_md_s
{
    double ShutterSpeedValue;
    double ApertureValue;
    uint32_t ISOSpeedRatings[3];
}ae_dng_md_t;

typedef struct blc_dng_md_s
{
    int32_t r_black_level;
    int32_t gr_black_level;
    int32_t gb_black_level;
    int32_t b_black_level;

    int32_t white_level;
}blc_dng_md_t;

typedef struct lsc_dng_md_s
{
    uint32_t img_w;
    uint32_t img_h;
    uint32_t crop_x;
    uint32_t crop_y;
    double FocusDistance;
    double FocalLength;
    double FocalPlaneXResolution;
    double FocalPlaneYResolution;
    uint32_t FocalPlaneResolutionUnit;
}lsc_dng_md_t;

typedef struct dng_md_s
{
    ae_dng_md_t ae_md;
    blc_dng_md_t blc_md;
    lsc_dng_md_t lsc_md;
}dng_md_t;