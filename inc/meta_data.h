#pragma once
#include <stdint.h>
#include <string>

typedef struct ae_dng_md_s
{
    double ExposureTime;
    double ApertureValue;
    uint32_t ISOSpeedRatings[3];
    double Bv;
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

typedef struct awb_dng_md_s
{
    double r_Neutral;
    double g_Neutral;
    double b_Neutral;

    bool asShot_valid;
    double asShot_x;
    double asShot_y;
}awb_dng_md_t;

typedef struct cc_dng_md_s
{
    double Analogbalance[3];
    uint32_t CalibrationIlluminant1;
    uint32_t CalibrationIlluminant2;
    double ColorMatrix1[3][3];
    double ColorMatrix2[3][3];
    double CameraCalibration1[3][3];
    double CameraCalibration2[3][3];
    double ForwardMatrix1[3][3];
    double ForwardMatrix2[3][3];
}cc_dng_md_t;

typedef struct ae_comp_md_s
{
    double BaselineExposure;
}ae_comp_md_t;

typedef struct sensor_crop_size_s {
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
}sensor_crop_size_t;

typedef struct dng_md_s
{
    std::string input_file_name;
    sensor_crop_size_t sensor_crop_size_info;
    ae_dng_md_t ae_md;
    blc_dng_md_t blc_md;
    lsc_dng_md_t lsc_md;
    awb_dng_md_t awb_md;
    cc_dng_md_t cc_md;
    ae_comp_md_t ae_comp_md;
}dng_md_t;