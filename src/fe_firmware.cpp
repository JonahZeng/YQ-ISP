#include "fe_firmware.h"
#include "meta_data.h"
#include "chromatix_lsc.h"
#include "chromatix_cc.h"
#include "dng_tag_values.h"
#include "dng_xy_coord.h"
#include "mat3_3lf.h"
#include <assert.h>

extern dng_md_t g_dng_all_md;

typedef struct global_ref_out_s
{
    double wp_x;
    double wp_y;
    double ccm[9];
    double FM[9];
}global_ref_out_t;

fe_firmware::fe_firmware(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

static void blc_reg_calc(dng_md_t& all_dng_md, blc_reg_t& blc_reg)
{
    blc_reg.bypass = 0;
    blc_reg.blc_r = all_dng_md.blc_md.r_black_level;
    blc_reg.blc_gr = all_dng_md.blc_md.gr_black_level;
    blc_reg.blc_gb = all_dng_md.blc_md.gb_black_level;
    blc_reg.blc_b = all_dng_md.blc_md.b_black_level;

    blc_reg.normalize_en = 1;
    blc_reg.white_level = (uint32_t)all_dng_md.blc_md.white_level;
}

typedef struct lsc_coeff_weight_s
{
    double a1;
    double a2;
    double a3;
    double weight;
}lsc_coeff_weight_t;

static void get_vignette_coeff_inner(double focusDist, lsc_coeff_element_t* element, lsc_coeff_weight_t* coeff_weight0, lsc_coeff_weight_t* coeff_weight1)
{

    log_info("focusDist %lf focusElement[0].focusDist %lf focusElement[5].focusDist %lf\n",
        focusDist, element->focusElement[0].focusDist, element->focusElement[5].focusDist);

    if (focusDist < element->focusElement[0].focusDist)
    {
        coeff_weight0->weight = 1.0;
        coeff_weight0->a1 = element->focusElement[0].a1;
        coeff_weight0->a2 = element->focusElement[0].a2;
        coeff_weight0->a3 = element->focusElement[0].a3;

        coeff_weight1->weight = 0.0;
        coeff_weight1->a1 = element->focusElement[0].a1;
        coeff_weight1->a2 = element->focusElement[0].a2;
        coeff_weight1->a3 = element->focusElement[0].a3;
    }
    else if (focusDist >= element->focusElement[5].focusDist)
    {
        coeff_weight0->weight = 0.0;
        coeff_weight0->a1 = element->focusElement[5].a1;
        coeff_weight0->a2 = element->focusElement[5].a2;
        coeff_weight0->a3 = element->focusElement[5].a3;

        coeff_weight1->weight = 1.0;
        coeff_weight1->a1 = element->focusElement[5].a1;
        coeff_weight1->a2 = element->focusElement[5].a2;
        coeff_weight1->a3 = element->focusElement[5].a3;
    }
    else
    {
        uint32_t i = 0;
        double total;
        double w1, w2;
        bool flag = false;
        for (; i < 5; i++)
        {
            if (focusDist >= element->focusElement[i].focusDist && focusDist < element->focusElement[i + 1].focusDist)
            {
                total = element->focusElement[i + 1].focusDist - element->focusElement[i].focusDist;
                w1 = element->focusElement[i + 1].focusDist - focusDist;
                w1 = w1 / total;
                w2 = 1.0 - w1;
                flag = true;
                break;
            }
        }
        if (!flag)
        {
            log_error("can't find neareset two focus dist\n");
            exit(1);
        }
        coeff_weight0->weight = w1;
        coeff_weight0->a1 = element->focusElement[i].a1;
        coeff_weight0->a2 = element->focusElement[i].a2;
        coeff_weight0->a3 = element->focusElement[i].a3;

        coeff_weight1->weight = w2;
        coeff_weight1->a1 = element->focusElement[i + 1].a1;
        coeff_weight1->a2 = element->focusElement[i + 1].a2;
        coeff_weight1->a3 = element->focusElement[i + 1].a3;
    }

    log_info("w1 %lf w2 %lf\n", coeff_weight0->weight, coeff_weight1->weight);
}

static void get_vignette_coeff(dng_md_t& all_dng_md, chromatix_lsc_t& lsc_calib, lsc_coeff_weight_t* coeff_weight)
{
    double focusDist = all_dng_md.lsc_md.FocusDistance;
    double apertureVal = all_dng_md.ae_md.ApertureValue;

    if (apertureVal < lsc_calib.element[0].apertureVal)
    {
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[0], coeff_weight, coeff_weight + 1);
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[1], coeff_weight + 2, coeff_weight + 3);
        (coeff_weight)->weight = (coeff_weight)->weight * 1.0;
        (coeff_weight + 1)->weight = (coeff_weight + 1)->weight * 1.0;
        (coeff_weight + 2)->weight = 0.0;
        (coeff_weight + 3)->weight = 0.0;
    }
    else if (apertureVal >= lsc_calib.element[3].apertureVal)
    {
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[2], coeff_weight, coeff_weight + 1);
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[3], coeff_weight + 2, coeff_weight + 3);
        (coeff_weight)->weight = 0.0;
        (coeff_weight + 1)->weight = 0.0;
        (coeff_weight + 2)->weight = (coeff_weight + 2)->weight * 1.0;
        (coeff_weight + 3)->weight = (coeff_weight + 3)->weight * 1.0;
    }
    else {
        int32_t i;
        bool flag = false;
        double total_step, part_step0;
        for (i = 0; i < 3; i++)
        {
            if (apertureVal >= lsc_calib.element[i].apertureVal && apertureVal < lsc_calib.element[i + 1].apertureVal)
            {
                flag = true;
                total_step = 1.0 / lsc_calib.element[i].apertureVal - 1.0 / lsc_calib.element[i + 1].apertureVal;
                part_step0 = 1.0 / apertureVal - 1.0 / lsc_calib.element[i + 1].apertureVal;
                log_info("apertureVal input %lf apertureVal0 %lf apertureVal1 %lf\n", apertureVal, lsc_calib.element[i].apertureVal, lsc_calib.element[i + 1].apertureVal);
                log_info("total_step %lf part_step0 %lf\n", total_step, part_step0);
                break;
            }
        }
        if (!flag)
        {
            log_error("can't find two neareset aperture value\n");
            exit(1);
        }

        get_vignette_coeff_inner(focusDist, &lsc_calib.element[i], coeff_weight, coeff_weight + 1);
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[i + 1], coeff_weight + 2, coeff_weight + 3);

        double weight0 = part_step0 / total_step;
        double weight1 = 1.0 - weight0;

        log_info("weight0 %lf weight1 %lf weight2 %lf weight3 %lf\n",
            (coeff_weight)->weight, (coeff_weight + 1)->weight, (coeff_weight + 2)->weight, (coeff_weight + 3)->weight);

        log_info("weight0 %lf weight1 %lf\n", weight0, weight1);

        (coeff_weight)->weight = (coeff_weight)->weight * weight0;
        (coeff_weight + 1)->weight = (coeff_weight + 1)->weight * weight0;
        (coeff_weight + 2)->weight = (coeff_weight + 2)->weight * weight1;
        (coeff_weight + 3)->weight = (coeff_weight + 3)->weight * weight1;
    }



    log_info("weight0 %lf weight1 %lf weight2 %lf weight3 %lf\n",
        (coeff_weight)->weight, (coeff_weight + 1)->weight, (coeff_weight + 2)->weight, (coeff_weight + 3)->weight);
}

static void lsc_reg_calc(dng_md_t& all_dng_md, lsc_reg_t& lsc_reg)
{
    chromatix_lsc_t lsc_calib =
    {
#include "calib_lsc_sigma_A102_nikonD610.h"
    };
    lsc_reg.block_size_x = all_dng_md.lsc_md.img_w / (LSC_GRID_COLS - 1);
    if (lsc_reg.block_size_x % 2 == 1)
    {
        lsc_reg.block_size_x += 1;
    }
    if (lsc_reg.block_size_x * (LSC_GRID_COLS - 1) < all_dng_md.lsc_md.img_w)
    {
        lsc_reg.block_size_x += 2;
    }

    lsc_reg.block_size_y = all_dng_md.lsc_md.img_h / (LSC_GRID_ROWS - 1);
    if (lsc_reg.block_size_y % 2 == 1)
    {
        lsc_reg.block_size_y += 1;
    }
    if (lsc_reg.block_size_y * (LSC_GRID_ROWS - 1) < all_dng_md.lsc_md.img_h)
    {
        lsc_reg.block_size_y += 2;
    }

    lsc_reg.block_start_x_oft = all_dng_md.lsc_md.crop_x % lsc_reg.block_size_x;
    lsc_reg.block_start_x_idx = all_dng_md.lsc_md.crop_x / lsc_reg.block_size_x;

    lsc_reg.block_start_y_oft = all_dng_md.lsc_md.crop_y % lsc_reg.block_size_y;
    lsc_reg.block_start_y_idx = all_dng_md.lsc_md.crop_y / lsc_reg.block_size_y;

    lsc_reg.bypass = 0;

    int32_t center_x_coordinate = (int32_t)(lsc_calib.center_x * all_dng_md.lsc_md.img_w);
    int32_t center_y_coordinate = (int32_t)(lsc_calib.center_y * all_dng_md.lsc_md.img_h);
    int32_t x, y;

    uint32_t resUnit = all_dng_md.lsc_md.FocalPlaneResolutionUnit;
    double resX, resY;
    if (resUnit == 3) //cm
    {
        resX = all_dng_md.lsc_md.FocalPlaneXResolution / 10;
        resY = all_dng_md.lsc_md.FocalPlaneYResolution / 10;
    }
    else if (resUnit == 2) //inch
    {
        resX = all_dng_md.lsc_md.FocalPlaneXResolution / 25.4;
        resY = all_dng_md.lsc_md.FocalPlaneYResolution / 25.4;
    }
    else {
        resX = all_dng_md.lsc_md.FocalPlaneXResolution;
        resY = all_dng_md.lsc_md.FocalPlaneYResolution;
    }


    lsc_coeff_weight_t coeff_weight[4] = { 0 };

    get_vignette_coeff(all_dng_md, lsc_calib, coeff_weight);

    for (uint32_t row = 0; row < LSC_GRID_ROWS; row++)
    {
        for (uint32_t col = 0; col < LSC_GRID_COLS; col++)
        {
            x = col * lsc_reg.block_size_x;
            y = row * lsc_reg.block_size_y;

            double r_x = double(x - center_x_coordinate) / (resX*35.0);
            double r_y = double(y - center_y_coordinate) / (resY*35.0);
            double r2 = r_x * r_x + r_y * r_y;
            double g0 = 1.0 + coeff_weight[0].a1 * r2 + coeff_weight[0].a2 * r2 * r2 + coeff_weight[0].a3 * r2 * r2 * r2;
            g0 = 1.0 / g0;
            double g1 = 1.0 + coeff_weight[1].a1 * r2 + coeff_weight[1].a2 * r2 * r2 + coeff_weight[1].a3 * r2 * r2 * r2;
            g1 = 1.0 / g1;
            double g2 = 1.0 + coeff_weight[2].a1 * r2 + coeff_weight[2].a2 * r2 * r2 + coeff_weight[2].a3 * r2 * r2 * r2;
            g2 = 1.0 / g2;
            double g3 = 1.0 + coeff_weight[3].a1 * r2 + coeff_weight[3].a2 * r2 * r2 + coeff_weight[3].a3 * r2 * r2 * r2;
            g3 = 1.0 / g3;

            double g = g0 * coeff_weight[0].weight + g1 * coeff_weight[1].weight + g2 * coeff_weight[2].weight + g3 * coeff_weight[3].weight;

            lsc_reg.luma_gain[row * LSC_GRID_COLS + col] = (uint32_t)(g * 1024);
        }
    }
}

static void ae_stat_reg_cal(dng_md_t& all_dng_md, ae_stat_reg_t& ae_stat_reg)
{
    ae_stat_reg.bypass = 0;
    ae_stat_reg.skip_step_x = 2;
    ae_stat_reg.skip_step_y = 2;
    ae_stat_reg.block_width = all_dng_md.sensor_crop_size_info.width / 32;
    if (ae_stat_reg.block_width % 2 == 1)
    {
        ae_stat_reg.block_width -= 1;
    }
    ae_stat_reg.block_height = all_dng_md.sensor_crop_size_info.height / 32;
    if (ae_stat_reg.block_height % 2 == 1)
    {
        ae_stat_reg.block_height -= 1;
    }

    double step_x_lf = double(all_dng_md.sensor_crop_size_info.width) / 32.0;
    double step_y_lf = double(all_dng_md.sensor_crop_size_info.height) / 32.0;
    for (int32_t i = 0; i < 32; i++)
    {
        double tmp_x = step_x_lf * i;
        tmp_x = round(tmp_x);
        uint32_t x_ = uint32_t(tmp_x);
        if (x_ % 2 == 1)
        {
            x_ += 1;
        }
        ae_stat_reg.start_x[i] = x_;
        double tmp_y = step_y_lf * i;
        tmp_y = round(tmp_y);
        uint32_t y_ = uint32_t(tmp_y);
        if (y_ % 2 == 1)
        {
            y_ += 1;
        }
        ae_stat_reg.start_y[i] = y_;
    }
}

static void awbgain_reg_calc(dng_md_t& all_dng_md, awbgain_reg_t& awbgain_reg)
{
    double r_gain = 1.0 / all_dng_md.awb_md.r_Neutral;
    double g_gain = 1.0 / all_dng_md.awb_md.g_Neutral;
    double b_gain = 1.0 / all_dng_md.awb_md.b_Neutral;

    awbgain_reg.bypass = 0;
    awbgain_reg.r_gain = uint32_t(r_gain * 1024);
    awbgain_reg.gr_gain = uint32_t(g_gain * 1024);
    awbgain_reg.gb_gain = awbgain_reg.gr_gain;
    awbgain_reg.b_gain = uint32_t(b_gain * 1024);

    double BaselineExposure = all_dng_md.ae_comp_md.BaselineExposure;
    double compensat_gain = pow(2, BaselineExposure);
    log_info("compensat_gain %lf, disable baseline exposure compensation\n", compensat_gain);

    awbgain_reg.ae_compensat_gain = 1024;// (uint32_t)(compensat_gain * 1024);
}

static void get_xy_wp(uint32_t CalibrationIlluminant1, uint32_t CalibrationIlluminant2, dng_xy_coord& xy1, dng_xy_coord& xy2)
{
    switch (CalibrationIlluminant1)
    {
    case lsStandardLightA:
        xy1 = StdA_xy_coord();
        break;
    case lsD55:
        xy1 = D55_xy_coord();
        break;
    case lsD65:
        xy1 = D65_xy_coord();
        break;
    case lsD50:
        xy1 = D50_xy_coord();
        break;
    case lsD75:
        xy1 = D75_xy_coord();
        break;
    default:
        xy1 = StdA_xy_coord();
        break;
    }

    switch (CalibrationIlluminant2)
    {
    case lsStandardLightA:
        xy2 = StdA_xy_coord();
        break;
    case lsD55:
        xy2 = D55_xy_coord();
        break;
    case lsD65:
        xy2 = D65_xy_coord();
        break;
    case lsD50:
        xy2 = D50_xy_coord();
        break;
    case lsD75:
        xy2 = D75_xy_coord();
        break;
    default:
        xy2 = StdA_xy_coord();
        break;
    }
}

static void calc_xy_coordinate_by_cameraNeutral(double* x, double* y, Vector3lf cameraNeutral,
    dng_xy_coord& wp1, dng_xy_coord& wp2,
    Mat3_3lf CM_1, Mat3_3lf CM_2, double* weight1, double* weight2)
{

    double white_point1_xy[2] = { wp1.x, wp1.y };
    double white_point2_xy[2] = { wp2.x, wp2.y };

    double dist_1_x, dist_1_y, dist_1;
    double dist_2_x, dist_2_y, dist_2;
    double weight_1, weight_2;

    double input_x = *x;
    double input_y = *y;

    double out_x = 0.0;
    double out_y = 0.0;

    for (int32_t i = 0; i < 25; i++)
    {
        log_info("-----------loop %d, input xy = %.6lf %.6lf\n", i, input_x, input_y);
        log_array("cameraNeutral:\n", "%.6lf, ", cameraNeutral.data, 3, 3);
        

        dist_1_x = abs(input_x - white_point1_xy[0]);
        dist_1_y = abs(input_y - white_point1_xy[1]);

        dist_1 = sqrt(dist_1_x * dist_1_x + dist_1_y * dist_1_y);

        dist_2_x = abs(input_x - white_point2_xy[0]);
        dist_2_y = abs(input_y - white_point2_xy[1]);

        dist_2 = sqrt(dist_2_x * dist_2_x + dist_2_y * dist_2_y);
        log_info("dist to 1 = %.6lf, to 2 = %.6lf\n", dist_1, dist_2);

        weight_1 = dist_1_x / (dist_1_x + dist_2_x);
        weight_2 = dist_2_x / (dist_1_x + dist_2_x);


        log_info("weight1 = %.6lf, weight2 = %.6lf\n", weight_1, weight_2);

        Mat3_3lf tmp_CM = CM_2 * weight_2 + CM_1 * weight_1;
        tmp_CM = tmp_CM.inv();

        log_array("tmp_CM:\n", "%.6lf, ", tmp_CM.get(), 9, 3);

        Vector3lf tmp_XYZ = tmp_CM.multiply(cameraNeutral);
        log_array("tmp_XYZ:\n", "%.6lf, ", tmp_XYZ.data, 3, 3);

        double X = tmp_XYZ.data[0];
        double Y = tmp_XYZ.data[1];
        double Z = tmp_XYZ.data[2];

        out_x = X / (X + Y + Z);
        out_y = Y / (X + Y + Z);

        log_info("out xy = %.6lf %.6lf\n", out_x, out_y);

        if (abs(out_x - input_x) < 0.01 &&  abs(out_y - input_y) < 0.01)
        {
            break;
        }
        else
        {
            input_x += (out_x - input_x) / 4;
            input_y += (out_y - input_y) / 4;
        }
    }
    *x = out_x;
    *y = out_y;
    *weight1 = weight_1;
    *weight1 = weight_2;
}

static void cc_reg_calc(dng_md_t& all_dng_md, cc_reg_t& cc_reg, global_ref_out_t& global_ref_out)
{
    chromatix_cc_t cc_calib = {
#include "calib_cc_sigma_A102_nikonD610.h"
    };
    if (cc_calib.cc_mode == 1)
    {
        Vector3lf cameraNeutral = { {all_dng_md.awb_md.r_Neutral, all_dng_md.awb_md.g_Neutral, all_dng_md.awb_md.b_Neutral} };

        if (all_dng_md.cc_md.Analogbalance[0] != 1.0 || all_dng_md.cc_md.Analogbalance[1] != 1.0 || all_dng_md.cc_md.Analogbalance[2] != 1.0)
        {
            log_error("AB is not all == 1.0\n");
        }
        if (all_dng_md.cc_md.CameraCalibration1[0][0] != 1.0 || all_dng_md.cc_md.CameraCalibration1[1][1] != 1.0
            || all_dng_md.cc_md.CameraCalibration1[2][2] != 1.0)
        {
            log_error("CC1 is not I matrix\n");
        }
        if (all_dng_md.cc_md.CameraCalibration2[0][0] != 1.0 || all_dng_md.cc_md.CameraCalibration2[1][1] != 1.0
            || all_dng_md.cc_md.CameraCalibration2[2][2] != 1.0)
        {
            log_error("CC2 is not I matrix\n");
        }

        Mat3_3lf CM_1;
        Mat3_3lf CM_2;
        Mat3_3lf FM_1;
        Mat3_3lf FM_2;

        for (int32_t row = 0; row < 3; row++)
        {
            for (int32_t col = 0; col < 3; col++)
            {
                CM_1.get()[row * 3 + col] = all_dng_md.cc_md.ColorMatrix1[row][col];
                CM_2.get()[row * 3 + col] = all_dng_md.cc_md.ColorMatrix2[row][col];

                FM_1.get()[row * 3 + col] = all_dng_md.cc_md.ForwardMatrix1[row][col];
                FM_2.get()[row * 3 + col] = all_dng_md.cc_md.ForwardMatrix1[row][col];
            }
        }

        uint32_t CalibrationIlluminant1 = all_dng_md.cc_md.CalibrationIlluminant1;
        uint32_t CalibrationIlluminant2 = all_dng_md.cc_md.CalibrationIlluminant2;
        assert((CalibrationIlluminant1 >= lsD55 && CalibrationIlluminant1 <= lsD50) || CalibrationIlluminant1 == lsStandardLightA);
        assert((CalibrationIlluminant2 >= lsD55 && CalibrationIlluminant2 <= lsD50) || CalibrationIlluminant2 == lsStandardLightA);
        dng_xy_coord wp1, wp2;
        get_xy_wp(CalibrationIlluminant1, CalibrationIlluminant2, wp1, wp2);

        double x = 0.390669;
        double y = 0.584906;
        double weight1 = 0.5;
        double weight2 = 0.5;
        calc_xy_coordinate_by_cameraNeutral(&x, &y, cameraNeutral, wp1, wp2,
            CM_1, CM_2, &weight1, &weight2);

        global_ref_out.wp_x = x;
        global_ref_out.wp_y = y;

        Mat3_3lf FM = FM_1 * weight1 + FM_2 * weight2;
        log_array("camera2XYZ_mat\n", "%lf, ", FM.get(), 9, 3);

        for (int32_t i = 0; i < 9; i++)
        {
            global_ref_out.FM[i] = FM.get()[i];
        }

        // ref: http://www.brucelindbloom.com/index.html?ColorCalculator.html
        
        Mat3_3lf XYZ2photoRGB (1.3459433, -0.2556075, -0.0511118,
            -0.5445989, 1.5081673, 0.0205351,
            0.0000000, 0.0000000, 1.2118128);
        Mat3_3lf ccm = XYZ2photoRGB.multiply(FM);
        //Mat3_3lf XYZ2RGB2020 = (
        //    1.7166512, -0.3556708, -0.2533663, 
        //    -0.66668445,  1.6164814,   0.01576855,
        //    0.01763986, -0.04277062,  0.9421031);
        //Mat3_3lf D50_to_D65 = (
        //    0.9857398,  0.0000000,  0.0000000,
        //    0.0000000,  1.0000000,  0.0000000,
        //    0.0000000,  0.0000000,  1.3194581
        //    );
        //Mat3_3lf ccm = XYZ2RGB2020.multiply((D50_to_D65.multiply(FM));
        log_array("sensor RGB to ProPhoto RGB ccm:\n", "%lf, ", ccm.get(), 9, 3);

        double* ccm_ptr = ccm.get();
        double row0_sum = ccm_ptr[0] + ccm_ptr[1] + ccm_ptr[2];
        ccm_ptr[0] = ccm_ptr[0] / row0_sum;
        ccm_ptr[1] = ccm_ptr[1] / row0_sum;
        ccm_ptr[2] = ccm_ptr[2] / row0_sum;
        double row1_sum = ccm_ptr[3] + ccm_ptr[4] + ccm_ptr[5];
        ccm_ptr[3] = ccm_ptr[3] / row1_sum;
        ccm_ptr[4] = ccm_ptr[4] / row1_sum;
        ccm_ptr[5] = ccm_ptr[5] / row1_sum;
        double row2_sum = ccm_ptr[6] + ccm_ptr[7] + ccm_ptr[8];
        ccm_ptr[6] = ccm_ptr[6] / row2_sum;
        ccm_ptr[7] = ccm_ptr[7] / row2_sum;
        ccm_ptr[8] = ccm_ptr[8] / row2_sum;

        cc_reg.bypass = 0;
        for (int32_t i = 0; i < 9; i++)
        {
            cc_reg.ccm[i] = (int32_t)(ccm.get()[i] * 1024);
            global_ref_out.ccm[i] = ccm.get()[i];
        }

        cc_reg.ccm[0] = 1024 - cc_reg.ccm[1] - cc_reg.ccm[2];
        cc_reg.ccm[4] = 1024 - cc_reg.ccm[3] - cc_reg.ccm[5];
        cc_reg.ccm[8] = 1024 - cc_reg.ccm[6] - cc_reg.ccm[7];

        log_array("final ccm:\n", "%d, ", cc_reg.ccm, 9, 3);
    }
}

static void hsv_lut_reg_calc(dng_md_t& g_dng_all_md, hsv_lut_reg_t& hsv_lut_reg, global_ref_out_t& global_ref_out)
{
    hsv_lut_reg.bypass = 0;
    hsv_lut_reg.twoD_enable = g_dng_all_md.hsv_lut_md.twoD_enable;
    hsv_lut_reg.threeD_enable = g_dng_all_md.hsv_lut_md.threeD_enable;
    hsv_lut_reg.twoD_map_encoding = g_dng_all_md.hsv_lut_md.fHueSatMapEncoding;
    hsv_lut_reg.threeD_map_encoding = g_dng_all_md.hsv_lut_md.fLookTableEncoding;

    uint32_t light1 = g_dng_all_md.hsv_lut_md.fCalibrationIlluminant1;
    uint32_t light2 = g_dng_all_md.hsv_lut_md.fCalibrationIlluminant2;
    dng_xy_coord wp1, wp2;
    get_xy_wp(light1, light2, wp1, wp2);

    double x1 = wp1.x - global_ref_out.wp_x;
    double y1 = wp1.y - global_ref_out.wp_y;
    double dist1 = sqrt(x1 * x1 + y1 * y1);
    double x2 = wp2.x - global_ref_out.wp_x;
    double y2 = wp2.y - global_ref_out.wp_y;
    double dist2 = sqrt(x2 * x2 + y2 * y2);

    double weight = dist1 / (dist1 + dist2);

    dng_hue_sat_map* result = dng_hue_sat_map::Interpolate(g_dng_all_md.hsv_lut_md.fHueSatDeltas1, g_dng_all_md.hsv_lut_md.fHueSatDeltas2, weight);
    if (result->IsValid())
    {
        uint32_t hD=0, sD=0, vD=0;
        result->GetDivisions(hD, sD, vD);
        hsv_lut_reg.twoDHueDivisions = hD;
        hsv_lut_reg.twoDSatDivisions = sD;

        memcpy(hsv_lut_reg.twoDmap, result->GetConstDeltas(), sizeof(dng_hue_sat_map::HSBModify) * hD * sD * vD);
    }
    delete result;

    uint32_t hD = 0, sD = 0, vD = 0;
    g_dng_all_md.hsv_lut_md.fLookTable.GetDivisions(hD, sD, vD);
    hsv_lut_reg.threeDHueDivisions = hD;
    hsv_lut_reg.threeDSatDivisions = sD;
    hsv_lut_reg.threeDValDivisions = vD;

    memcpy(hsv_lut_reg.threeDmap, g_dng_all_md.hsv_lut_md.fLookTable.GetConstDeltas(), sizeof(dng_hue_sat_map::HSBModify) * hD * sD * vD);
}

static void prophoto2srgb_reg_calc(prophoto2srgb_reg_t& prophoto2srgb_reg, global_ref_out_t& global_ref_out)
{
    Mat3_3lf XYZ2sRGB(
        3.2404542, -1.5371385, -0.4985314,
        -0.9692660, 1.8760108, 0.0415560,
        0.0556434, -0.2040259, 1.0572252);
    Mat3_3lf D50_to_D65(
        0.9857398,  0.0000000,  0.0000000,
        0.0000000,  1.0000000,  0.0000000,
        0.0000000,  0.0000000,  1.3194581
        );
    Mat3_3lf FM(global_ref_out.FM);
    Mat3_3lf old_ccm(global_ref_out.ccm);
    Mat3_3lf iccm = old_ccm.inv();
    Mat3_3lf ccm = XYZ2sRGB.multiply(D50_to_D65.multiply(FM.multiply(iccm)));

    double* ccm_ptr = ccm.get();
    double row0_sum = ccm_ptr[0] + ccm_ptr[1] + ccm_ptr[2];
    ccm_ptr[0] = ccm_ptr[0] / row0_sum;
    ccm_ptr[1] = ccm_ptr[1] / row0_sum;
    ccm_ptr[2] = ccm_ptr[2] / row0_sum;
    double row1_sum = ccm_ptr[3] + ccm_ptr[4] + ccm_ptr[5];
    ccm_ptr[3] = ccm_ptr[3] / row1_sum;
    ccm_ptr[4] = ccm_ptr[4] / row1_sum;
    ccm_ptr[5] = ccm_ptr[5] / row1_sum;
    double row2_sum = ccm_ptr[6] + ccm_ptr[7] + ccm_ptr[8];
    ccm_ptr[6] = ccm_ptr[6] / row2_sum;
    ccm_ptr[7] = ccm_ptr[7] / row2_sum;
    ccm_ptr[8] = ccm_ptr[8] / row2_sum;

    prophoto2srgb_reg.bypass = 0;
    prophoto2srgb_reg.hlr_en = 1;
    for (int32_t i = 0; i < 9; i++)
    {
        prophoto2srgb_reg.ccm[i] = (int32_t)(ccm.get()[i] * 1024);
    }

    prophoto2srgb_reg.ccm[0] = 1024 - prophoto2srgb_reg.ccm[1] - prophoto2srgb_reg.ccm[2];
    prophoto2srgb_reg.ccm[4] = 1024 - prophoto2srgb_reg.ccm[3] - prophoto2srgb_reg.ccm[5];
    prophoto2srgb_reg.ccm[8] = 1024 - prophoto2srgb_reg.ccm[6] - prophoto2srgb_reg.ccm[7];

    log_array("prophoto2srgb ccm:\n", "%d, ", prophoto2srgb_reg.ccm, 9, 3);
}

static void gtm_reg_calc(dng_md_t& all_dng_md, statistic_info_t* stat_out, gtm_reg_t& gtm_reg, uint32_t frame_cnt)
{
    //uint32_t rgb2y[3] = { 306, 601, 117 };
    //uint32_t rgb2y[3] = { 217,  732,  75 };
    gtm_reg.bypass = 0;
    gtm_reg.rgb2y[0] = 217;
    gtm_reg.rgb2y[1] = 732;
    gtm_reg.rgb2y[2] = 75;
    if (frame_cnt == 0 || stat_out->gtm_stat.stat_en == 0)
    {
        uint32_t gain_map[257] =
        {
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
        1024, 1024, 1024, 1024, 1024
        };
        memcpy(gtm_reg.gain_lut, gain_map, sizeof(uint32_t) * 257);
    }
    else 
    {
        uint32_t* luma_hist = stat_out->gtm_stat.luma_hist;
        if(stat_out->gtm_stat.stat_en == 0)
        {
            uint32_t gain_map[257] =
            {
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                1024, 1024, 1024, 1024, 1024
            };
            memcpy(gtm_reg.gain_lut, gain_map, sizeof(uint32_t) * 257);
        }
        else
        {
            uint32_t luma_sum = 0;
            for (uint32_t i = 0; i < 256; i++)
            {
                luma_sum += luma_hist[i] * i;
            }
            uint32_t luma_mean = luma_sum / stat_out->gtm_stat.total_pixs;

            double Bv = all_dng_md.ae_md.Bv;
            log_debug("luma mean = %d, Bv=%lf\n", luma_mean, Bv);

            double Bv_thr[10] = { -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
            double luma_tar[10] = { 15.0, 25.0, 35.0, 45.0, 45.0, 50.0, 55.0, 55.0, 55.0, 55.0 };

            int32_t idx0, idx1;
            double luma_out = 0.0;
            if (Bv < Bv_thr[0])
            {
                idx0 = 0;
                idx1 = 0;
                luma_out = luma_tar[0];
            }
            else if (Bv >= Bv_thr[9])
            {
                idx0 = 9;
                idx1 = 9;
                luma_out = luma_tar[9];
            }
            else
            {
                int32_t i = 0;
                for (; i < 9; i++)
                {
                    if (Bv >= Bv_thr[i] && Bv < Bv_thr[i + 1])
                    {
                        break;
                    }
                }
                idx0 = i;
                idx1 = i + 1;
                double delta = Bv - Bv_thr[idx0];
                double step = Bv_thr[idx1] - Bv_thr[idx0];

                luma_out = luma_tar[idx0] + (luma_tar[idx1] - luma_tar[idx0]) * delta / step;
            }

            double global_gain = luma_out / luma_mean;
            uint32_t gain1p5[257] = {
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
                1536, 1536, 1536, 1536, 1536, 1535, 1535, 1535, 1535, 1535,
                1534, 1534, 1533, 1532, 1532, 1531, 1530, 1530, 1529, 1528,
                1527, 1526, 1525, 1524, 1522, 1521, 1520, 1519, 1517, 1516,
                1515, 1513, 1512, 1510, 1509, 1507, 1505, 1504, 1502, 1500,
                1499, 1497, 1495, 1493, 1492, 1490, 1488, 1486, 1484, 1482,
                1480, 1478, 1476, 1474, 1472, 1470, 1468, 1466, 1463, 1461,
                1459, 1457, 1455, 1452, 1450, 1448, 1445, 1443, 1441, 1438,
                1436, 1433, 1431, 1429, 1426, 1424, 1421, 1419, 1416, 1414,
                1411, 1408, 1406, 1403, 1401, 1398, 1395, 1393, 1390, 1387,
                1384, 1382, 1379, 1376, 1373, 1371, 1368, 1365, 1362, 1359,
                1356, 1353, 1351, 1348, 1345, 1342, 1339, 1336, 1333, 1330,
                1327, 1324, 1321, 1318, 1315, 1312, 1308, 1305, 1302, 1299,
                1296, 1293, 1290, 1287, 1283, 1280, 1277, 1274, 1270, 1267,
                1264, 1261, 1257, 1254, 1251, 1247, 1244, 1241, 1237, 1234,
                1231, 1227, 1224, 1220, 1217, 1214, 1210, 1207, 1203, 1200,
                1196, 1193, 1189, 1186, 1182, 1179, 1175, 1171, 1168, 1164,
                1161, 1157, 1153, 1150, 1146, 1142, 1139, 1135, 1131, 1128,
                1124, 1120, 1116, 1113, 1109, 1105, 1101, 1098, 1094, 1090,
                1086, 1082, 1078, 1075, 1071, 1067, 1063, 1059, 1055, 1051,
                1047, 1043, 1039, 1035, 1031, 1027, 1024 };

            uint32_t gain2[257] = {
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                2048, 2048, 2048, 2048, 2048, 2048, 2048, 2047, 2047,
                2046, 2045, 2044, 2042, 2041, 2039, 2036, 2034, 2031,
                2028, 2025, 2022, 2019, 2016, 2012, 2008, 2004, 2001,
                1996, 1992, 1988, 1984, 1979, 1975, 1970, 1965, 1960,
                1956, 1951, 1946, 1941, 1935, 1930, 1925, 1920, 1914,
                1909, 1904, 1898, 1893, 1887, 1882, 1876, 1870, 1865,
                1859, 1853, 1848, 1842, 1836, 1830, 1824, 1819, 1813,
                1807, 1801, 1795, 1789, 1783, 1777, 1771, 1765, 1759,
                1753, 1747, 1741, 1735, 1729, 1723, 1718, 1712, 1706,
                1700, 1694, 1687, 1682, 1676, 1670, 1664, 1658, 1652,
                1646, 1640, 1634, 1628, 1622, 1616, 1610, 1604, 1598,
                1592, 1586, 1580, 1574, 1568, 1562, 1557, 1551, 1545,
                1539, 1533, 1527, 1522, 1516, 1510, 1504, 1498, 1493,
                1487, 1481, 1475, 1470, 1464, 1458, 1453, 1447, 1441,
                1436, 1430, 1425, 1419, 1414, 1408, 1402, 1397, 1391,
                1386, 1381, 1375, 1370, 1364, 1359, 1353, 1348, 1343,
                1337, 1332, 1327, 1321, 1316, 1311, 1306, 1301, 1295,
                1290, 1285, 1280, 1275, 1270, 1265, 1260, 1254, 1249,
                1244, 1239, 1235, 1230, 1225, 1220, 1215, 1210, 1205,
                1200, 1195, 1191, 1186, 1181, 1176, 1172, 1167, 1162,
                1158, 1153, 1148, 1144, 1139, 1135, 1130, 1126, 1121,
                1117, 1112, 1108, 1103, 1099, 1095, 1090, 1086, 1082,
                1077, 1073, 1069, 1065, 1060, 1056, 1052, 1048, 1044,
                1040, 1036, 1032, 1028, 1024 };

            uint32_t gain3[257] = {
                3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3071,
                3068, 3064, 3058, 3051, 3044, 3035, 3027, 3018, 3008, 2998, 2988, 2977,
                2967, 2956, 2945, 2934, 2923, 2912, 2900, 2889, 2877, 2866, 2854, 2843,
                2831, 2820, 2808, 2796, 2785, 2773, 2761, 2750, 2738, 2726, 2715, 2703,
                2691, 2680, 2668, 2657, 2645, 2634, 2622, 2610, 2599, 2588, 2576, 2565,
                2553, 2542, 2531, 2519, 2508, 2497, 2486, 2474, 2463, 2452, 2441, 2430,
                2419, 2408, 2397, 2386, 2375, 2364, 2353, 2342, 2332, 2321, 2310, 2300,
                2289, 2278, 2268, 2257, 2247, 2236, 2226, 2215, 2205, 2195, 2184, 2174,
                2164, 2154, 2144, 2134, 2124, 2113, 2103, 2094, 2084, 2074, 2064, 2054,
                2044, 2035, 2025, 2015, 2006, 1996, 1986, 1977, 1967, 1958, 1949, 1939,
                1930, 1921, 1911, 1902, 1893, 1884, 1875, 1866, 1857, 1848, 1839, 1830,
                1821, 1812, 1803, 1795, 1786, 1777, 1769, 1760, 1752, 1743, 1735, 1726,
                1718, 1709, 1701, 1693, 1684, 1676, 1668, 1660, 1652, 1644, 1636, 1628,
                1620, 1612, 1604, 1596, 1589, 1581, 1573, 1565, 1558, 1550, 1543, 1535,
                1528, 1520, 1513, 1505, 1498, 1491, 1484, 1476, 1469, 1462, 1455, 1448,
                1441, 1434, 1427, 1420, 1413, 1407, 1400, 1393, 1386, 1380, 1373, 1367,
                1360, 1354, 1347, 1341, 1334, 1328, 1322, 1315, 1309, 1303, 1297, 1291,
                1285, 1279, 1273, 1267, 1261, 1255, 1249, 1243, 1238, 1232, 1226, 1221,
                1215, 1209, 1204, 1198, 1193, 1188, 1182, 1177, 1172, 1166, 1161, 1156,
                1151, 1146, 1141, 1136, 1131, 1126, 1121, 1116, 1111, 1107, 1102, 1097,
                1093, 1088, 1083, 1079, 1074, 1070, 1065, 1061, 1057, 1052, 1048, 1044,
                1040, 1036, 1032, 1028, 1024};

            if (global_gain < 1.0)
            {
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg.gain_lut[i] = uint32_t(1024 * global_gain + 0.5);
                }
            } 
            else if (global_gain >= 1.0 && global_gain < 1.5)
            {
                double w1 = global_gain - 1.0;
                
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg.gain_lut[i] = uint32_t(1024 + (gain1p5[i] - 1024)*w1/0.5);
                }
            }
            else if (global_gain >= 1.5 && global_gain < 2.0)
            {
                double w1 = global_gain - 1.5;

                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg.gain_lut[i] = uint32_t(gain1p5[i] + (gain2[i] - gain1p5[i]) * w1 / 0.5);
                }
            }
            else if (global_gain >= 2.0 && global_gain < 3.0)
            {
                double w1 = global_gain - 2.0;

                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg.gain_lut[i] = uint32_t(gain2[i] + (gain3[i] - gain2[i]) * w1);
                }
            }
            else
            {
                for (int32_t i = 0; i < 257; i++)
                {
                    gtm_reg.gain_lut[i] = gain3[i];
                }
            }
        }
    }
}

static void gtm_stat_reg_calc(gtm_stat_reg_t& gtm_stat_reg)
{
    gtm_stat_reg.bypass = 0;
    gtm_stat_reg.rgb2y[0] = 217;
    gtm_stat_reg.rgb2y[1] = 732;
    gtm_stat_reg.rgb2y[2] = 75;
    gtm_stat_reg.w_ratio = 4;
    gtm_stat_reg.h_ratio = 4;
}

static void gamma_reg_calc(gamma_reg_t& gamma_reg)
{
    gamma_reg.bypass = 0;
    uint16_t gamma_22[257] = {
     0,   201,   398,   591,   781,   967,  1149,  1328,  1504,  1677,
  1846,  2012,  2175,  2335,  2492,  2646,  2797,  2945,  3091,  3234,
  3374,  3512,  3647,  3780,  3911,  4039,  4165,  4288,  4410,  4530,
  4647,  4763,  4877,  4989,  5099,  5207,  5314,  5420,  5524,  5626,
  5727,  5827,  5926,  6023,  6120,  6215,  6309,  6403,  6495,  6587,
  6678,  6768,  6858,  6947,  7036,  7124,  7212,  7299,  7386,  7473,
  7559,  7644,  7729,  7814,  7898,  7981,  8064,  8146,  8228,  8309,
  8389,  8469,  8548,  8627,  8705,  8782,  8858,  8934,  9009,  9083,
  9157,  9230,  9302,  9373,  9443,  9513,  9582,  9650,  9717,  9783,
  9849,  9913,  9977, 10041, 10103, 10165, 10226, 10286, 10346, 10405,
 10464, 10521, 10578, 10635, 10691, 10746, 10801, 10855, 10909, 10962,
 11015, 11067, 11118, 11170, 11220, 11271, 11320, 11370, 11419, 11467,
 11516, 11564, 11611, 11658, 11705, 11752, 11798, 11844, 11890, 11935,
 11980, 12025, 12070, 12114, 12158, 12202, 12245, 12288, 12331, 12374,
 12416, 12458, 12500, 12542, 12583, 12625, 12665, 12706, 12746, 12787,
 12826, 12866, 12906, 12945, 12984, 13023, 13061, 13099, 13137, 13175,
 13213, 13250, 13288, 13325, 13361, 13398, 13434, 13470, 13506, 13542,
 13578, 13613, 13649, 13684, 13719, 13753, 13788, 13822, 13856, 13891,
 13925, 13958, 13992, 14026, 14059, 14092, 14125, 14159, 14191, 14224,
 14257, 14290, 14322, 14355, 14387, 14419, 14451, 14483, 14515, 14547,
 14579, 14611, 14643, 14674, 14706, 14738, 14769, 14801, 14832, 14864,
 14895, 14927, 14958, 14989, 15021, 15052, 15083, 15115, 15146, 15178,
 15209, 15240, 15272, 15303, 15335, 15366, 15398, 15430, 15461, 15493,
 15525, 15557, 15588, 15620, 15652, 15685, 15717, 15749, 15781, 15814,
 15846, 15879, 15912, 15945, 15978, 16011, 16044, 16077, 16111, 16144,
 16178, 16212, 16246, 16280, 16314, 16349, 16383
    };
    //uint16_t gamma_log[257] = {
    //     2038,  4190,  5176,  5885,  6455,  6939,  7366,  7737,  8057,  8340,
    //     8594,  8822,  9031,  9224,  9402,  9568,  9723,  9868, 10005, 10135,
    //    10259, 10376, 10488, 10594, 10697, 10795, 10889, 10979, 11067, 11151,
    //    11233, 11311, 11388, 11462, 11533, 11603, 11671, 11736, 11800, 11863,
    //    11924, 11983, 12041, 12097, 12153, 12207, 12259, 12311, 12362, 12411,
    //    12460, 12507, 12554, 12600, 12645, 12689, 12732, 12774, 12816, 12857,
    //    12898, 12937, 12976, 13015, 13053, 13090, 13127, 13163, 13198, 13233,
    //    13268, 13302, 13336, 13369, 13401, 13434, 13466, 13497, 13528, 13559,
    //    13589, 13619, 13648, 13677, 13706, 13734, 13762, 13790, 13818, 13845,
    //    13872, 13898, 13924, 13950, 13976, 14002, 14027, 14052, 14076, 14101,
    //    14125, 14149, 14172, 14196, 14219, 14242, 14265, 14287, 14310, 14332,
    //    14354, 14375, 14397, 14418, 14440, 14461, 14481, 14502, 14522, 14543,
    //    14563, 14583, 14602, 14622, 14642, 14661, 14680, 14699, 14718, 14736,
    //    14755, 14773, 14792, 14810, 14828, 14846, 14863, 14881, 14898, 14916,
    //    14933, 14950, 14967, 14984, 15001, 15017, 15034, 15050, 15067, 15083,
    //    15099, 15115, 15131, 15146, 15162, 15178, 15193, 15208, 15224, 15239,
    //    15254, 15269, 15284, 15298, 15313, 15328, 15342, 15357, 15371, 15385,
    //    15399, 15414, 15428, 15441, 15455, 15469, 15483, 15496, 15510, 15523,
    //    15537, 15550, 15563, 15576, 15590, 15603, 15616, 15628, 15641, 15654,
    //    15667, 15679, 15692, 15704, 15717, 15729, 15741, 15754, 15766, 15778,
    //    15790, 15802, 15814, 15826, 15837, 15849, 15861, 15872, 15884, 15896,
    //    15907, 15918, 15930, 15941, 15952, 15964, 15975, 15986, 15997, 16008,
    //    16019, 16030, 16041, 16051, 16062, 16073, 16083, 16094, 16105, 16115,
    //    16126, 16136, 16146, 16157, 16167, 16177, 16187, 16198, 16208, 16218,
    //    16228, 16238, 16248, 16258, 16268, 16277, 16287, 16297, 16307, 16316,
    //    16326, 16335, 16345, 16355, 16364, 16373, 16383 };
    for(int32_t i=0; i<257; i++)
    {
        gamma_reg.gamma_lut[i] = gamma_22[i];
        //gamma_reg.gamma_lut[i] = gamma_log[i];
    }
}

static void rgb2yuv_reg_calc(rgb2yuv_reg_t& rgb2yuv_reg)
{
    rgb2yuv_reg.bypass = 0;
    rgb2yuv_reg.rgb2yuv_coeff[0] = 306;// 0.299⋅R + 0.587⋅G + 0.114 B
    rgb2yuv_reg.rgb2yuv_coeff[1] = 601;
    rgb2yuv_reg.rgb2yuv_coeff[2] = 117;

    rgb2yuv_reg.rgb2yuv_coeff[3] = -173;
    rgb2yuv_reg.rgb2yuv_coeff[4] = -339;
    rgb2yuv_reg.rgb2yuv_coeff[5] = 512;

    rgb2yuv_reg.rgb2yuv_coeff[6] = 512;
    rgb2yuv_reg.rgb2yuv_coeff[7] = -429;
    rgb2yuv_reg.rgb2yuv_coeff[8] = -83;
}

static void sensor_crop_reg_calc(dng_md_t& all_dng_md, sensor_crop_reg_t& sensorcrop_reg)
{
    sensorcrop_reg.bypass = 0;
    sensorcrop_reg.origin_x = all_dng_md.sensor_crop_size_info.origin_x;
    sensorcrop_reg.origin_y = all_dng_md.sensor_crop_size_info.origin_y;
    sensorcrop_reg.width = all_dng_md.sensor_crop_size_info.width;
    sensorcrop_reg.height = all_dng_md.sensor_crop_size_info.height;
}

static void yuv422_conv_reg_calc(yuv422_conv_reg_t& yuv422_conv_reg)
{
    yuv422_conv_reg.filter_coeff[0] = 2;
    yuv422_conv_reg.filter_coeff[1] = 6;
    yuv422_conv_reg.filter_coeff[2] = 6;
    yuv422_conv_reg.filter_coeff[3] = 2;
}

void fe_firmware::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);

    data_buffer* input = in[0];
    data_buffer* output0 = new data_buffer(input->width, input->height, input->data_type, input->bayer_pattern, "fe_fw_out0");

    memcpy(output0->data_ptr, input->data_ptr, input->width*input->height * sizeof(uint16_t));

    size_t reg_len = sizeof(fe_module_reg_t) / sizeof(uint16_t);
    if (reg_len * sizeof(uint16_t) < sizeof(fe_module_reg_t))
    {
        reg_len += 1;
    }
    data_buffer* output1 = new data_buffer((uint32_t)reg_len, 1, DATA_TYPE_RAW, BAYER_UNSUPPORT, "fe_fw_out1");
    fe_module_reg_t* reg_ptr = (fe_module_reg_t*)output1->data_ptr;

    out[0] = output0;
    out[1] = output1;

    global_ref_out_t global_ref_out;

    global_ref_out.wp_x = 0.0;
    global_ref_out.wp_y = 0.0; //for hs map interpolate

    sensor_crop_reg_calc(g_dng_all_md, reg_ptr->sensor_crop_reg);
    blc_reg_calc(g_dng_all_md, reg_ptr->blc_reg);
    lsc_reg_calc(g_dng_all_md, reg_ptr->lsc_reg);
    ae_stat_reg_cal(g_dng_all_md, reg_ptr->ae_stat_reg);
    awbgain_reg_calc(g_dng_all_md, reg_ptr->awbgain_reg);
    cc_reg_calc(g_dng_all_md, reg_ptr->cc_reg, global_ref_out);
    hsv_lut_reg_calc(g_dng_all_md, reg_ptr->hsv_lut_reg, global_ref_out);
    prophoto2srgb_reg_calc(reg_ptr->prophoto2srgb_reg, global_ref_out);
    gtm_stat_reg_calc(reg_ptr->gtm_stat_reg);
    gtm_reg_calc(g_dng_all_md, stat_out, reg_ptr->gtm_reg, frame_cnt);
    gamma_reg_calc(reg_ptr->gamma_reg);
    rgb2yuv_reg_calc(reg_ptr->rgb2yuv_reg);
    yuv422_conv_reg_calc(reg_ptr->yuv422_conv_reg);

    hw_base::hw_run(stat_out, frame_cnt);

    log_info("%s run end\n", __FUNCTION__);
}

void fe_firmware::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",     UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

fe_firmware::~fe_firmware()
{
    log_info("%s module %s deinit start\n", __FUNCTION__, name);
    log_info("%s module %s deinit end\n", __FUNCTION__, name);
};
