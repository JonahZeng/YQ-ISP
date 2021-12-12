#include "fe_firmware.h"
#include "meta_data.h"
#include "chromatix_lsc.h"
#include "chromatix_cc.h"
#include "dng_tag_values.h"
#include "dng_xy_coord.h"
#include "opencv2/opencv.hpp"

extern dng_md_t g_dng_all_md;

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
    log_info("compensat_gain %lf\n", compensat_gain);

    awbgain_reg.ae_compensat_gain = (uint32_t)(compensat_gain * 1024);
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

static void calc_xy_coordinate_by_cameraNeutral(double* x, double* y, cv::Mat cameraNeutral,
    dng_xy_coord& wp1, dng_xy_coord& wp2,
    cv::Mat CM_1, cv::Mat CM_2, double* weight1, double* weight2)
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
        log_array("cameraNeutral:\n", "%.6lf, ", cameraNeutral.ptr<double>(), (uint32_t)cameraNeutral.cols*cameraNeutral.rows, (uint32_t)cameraNeutral.cols);
        //std::cout << "cameraNeutral:" << cameraNeutral << std::endl;

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

        cv::Mat tmp_CM = CM_2 * weight_2 + CM_1 * weight_1;
        tmp_CM = tmp_CM.inv();

        //std::cout << "tmp_CM:" << tmp_CM << std::endl;
        log_array("tmp_CM:\n", "%.6lf, ", tmp_CM.ptr<double>(), (uint32_t)tmp_CM.cols*tmp_CM.rows, (uint32_t)tmp_CM.cols);

        cv::Mat tmp_XYZ = tmp_CM * cameraNeutral;
        //std::cout << "tmp_XYZ:" << tmp_XYZ << std::endl;
        log_array("tmp_XYZ:\n", "%.6lf, ", tmp_XYZ.ptr<double>(), (uint32_t)tmp_XYZ.cols*tmp_XYZ.rows, (uint32_t)tmp_XYZ.cols);

        double X = tmp_XYZ.at<double>(0);
        double Y = tmp_XYZ.at<double>(1);
        double Z = tmp_XYZ.at<double>(2);

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

static void cc_reg_calc(dng_md_t& all_dng_md, cc_reg_t& cc_reg)
{
    chromatix_cc_t cc_calib = {
#include "calib_cc_sigma_A102_nikonD610.h"
    };
    if (cc_calib.cc_mode == 1)
    {
        cv::Mat cameraNeutral = (cv::Mat_<double>(3, 1) << all_dng_md.awb_md.r_Neutral, all_dng_md.awb_md.g_Neutral, all_dng_md.awb_md.b_Neutral);

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

        cv::Mat CM_1(3, 3, CV_64FC1);
        cv::Mat CM_2(3, 3, CV_64FC1);
        cv::Mat FM_1(3, 3, CV_64FC1);
        cv::Mat FM_2(3, 3, CV_64FC1);

        for (int32_t row = 0; row < 3; row++)
        {
            for (int32_t col = 0; col < 3; col++)
            {
                CM_1.at<double>(row, col) = all_dng_md.cc_md.ColorMatrix1[row][col];
                CM_2.at<double>(row, col) = all_dng_md.cc_md.ColorMatrix2[row][col];

                FM_1.at<double>(row, col) = all_dng_md.cc_md.ForwardMatrix1[row][col];
                FM_2.at<double>(row, col) = all_dng_md.cc_md.ForwardMatrix1[row][col];
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

        cv::Mat FM = FM_1 * weight1 + FM_2 * weight2;
        log_array("camera2XYZ_mat\n", "%lf, ", FM.ptr<double>(), (uint32_t)FM.cols*FM.rows, (uint32_t)FM.cols);

        cv::Mat XYZ2sRGB = (cv::Mat_<double>(3, 3) <<
            3.2404542, -1.5371385, -0.4985314,
            -0.9692660, 1.8760108, 0.0415560,
            0.0556434, -0.2040259, 1.0572252);
        cv::Mat ccm = XYZ2sRGB * FM;
        //cv::Mat XYZ2photoRGB = (cv::Mat_<double>(3, 3) <<
        //    1.3459433, -0.2556075, -0.0511118,
        //    -0.5445989, 1.5081673, 0.0205351,
        //    0.0000000, 0.0000000, 1.2118128);
        //cv::Mat ccm = XYZ2photoRGB * FM;
        log_array("ccm:\n", "%lf, ", ccm.ptr<double>(), (uint32_t)ccm.cols*FM.rows, (uint32_t)ccm.cols);

        double* ccm_ptr = ccm.ptr<double>();
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
            cc_reg.ccm[i] = (int32_t)(ccm.at<double>(i / 3, i % 3) * 1024);
        }

        cc_reg.ccm[0] = 1024 - cc_reg.ccm[1] - cc_reg.ccm[2];
        cc_reg.ccm[4] = 1024 - cc_reg.ccm[3] - cc_reg.ccm[5];
        cc_reg.ccm[8] = 1024 - cc_reg.ccm[6] - cc_reg.ccm[7];
    }
}

static void gtm_reg_calc(statistic_info_t* stat_out, gtm_reg_t& gtm_reg, uint32_t frame_cnt)
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
#ifdef _MSC_VER
        memcpy_s(gtm_reg.gain_lut, sizeof(uint32_t) * 257, gain_map, sizeof(uint32_t) * 257);
#else
        memcpy(gtm_reg.gain_lut, gain_map, sizeof(uint32_t) * 257);
#endif
    }
    else 
    {
        const uint32_t* luma_hist = stat_out->gtm_stat.luma_hist;
        uint32_t total_cnt = stat_out->gtm_stat.total_pixs;
        uint32_t hist_mean_val = total_cnt / 256;
        
        uint64_t square_sum = 0;
        uint32_t great_bins_cnt = 0;
        for (int32_t i = 0; i < 256; i++)
        {
            if (luma_hist[i] > hist_mean_val)
            {
                square_sum += (luma_hist[i] - hist_mean_val) * (luma_hist[i] - hist_mean_val);
                great_bins_cnt += 1;
            }
        }
        square_sum = square_sum / great_bins_cnt;
        uint32_t std_err = (uint32_t)sqrt(square_sum);

        uint32_t* hist_adapt = new uint32_t[256];
        square_sum = 0;
        for (int32_t i = 0; i < 256; i++)
        {
            if (luma_hist[i] > (hist_mean_val + std_err))
            {
                hist_adapt[i] = hist_mean_val + std_err;
                square_sum += luma_hist[i] - (hist_mean_val + std_err);
            }
            else
            {
                hist_adapt[i] = luma_hist[i];
            }
        }
        uint32_t low_addition = (uint32_t)(square_sum + 128)/ 256;
        for (int32_t i = 0; i < 256; i++)
        {
            hist_adapt[i] += low_addition;
        }

        uint32_t* hist_equal = new uint32_t[256];
        double* hist_equal_db = new double[256 + 8]; //savgol window size=9, order=3
        double* hist_equal_res = new double[256];

        hist_equal[0] = hist_adapt[0];
        
        for (int32_t i = 1; i < 256; i++)
        {
            hist_equal[i] = hist_equal[i - 1] + hist_adapt[i];
        }
        //assert(hist_equal[255] == total_cnt);

        
        for (int32_t i = 0; i < 256; i++)
        {
            hist_equal_db[i + 4] = double(hist_equal[i]) * 255.0 / hist_equal[255];
        }
        for (int32_t i = 0; i < 4; i++)
        {
            hist_equal_db[i] = hist_equal_db[8 - i];
        }
        for (int32_t i = 256 + 4; i < 256 + 8; i++)
        {
            hist_equal_db[i] = hist_equal_db[259 * 2 - i];
        }

        double coeff[9] = {-0.09090909, 0.06060606, 0.16883117, 0.23376623, 0.25541126, 0.23376623, 0.16883117, 0.06060606, -0.09090909};

        for (int32_t i = 0; i < 256; i++)
        {
            hist_equal_res[i] = hist_equal_db[i + 0] * coeff[0] + hist_equal_db[i + 1] * coeff[1] + hist_equal_db[i + 2] * coeff[2] +
                                hist_equal_db[i + 3] * coeff[3] + hist_equal_db[i + 4] * coeff[4] + hist_equal_db[i + 5] * coeff[5] +
                                hist_equal_db[i + 6] * coeff[6] + hist_equal_db[i + 7] * coeff[7] + hist_equal_db[i + 8] * coeff[8];
            if (hist_equal_res[i] < 0.0)
            {
                hist_equal_res[i] = 0.0;
            }
        }

        uint32_t gain_map[257] = { 0 };

        for (int32_t i = 1; i < 256; i++)
        {
            hist_equal_res[i] = hist_equal_res[i] * 1024 / i;
            gain_map[i] = uint32_t(hist_equal_res[i]);
        }
        gain_map[0] = 0;
        gain_map[256] = gain_map[255];
        log_array("gain_map:\n", "%6d, ", gain_map, 257, 16);
#ifdef _MSC_VER
        memcpy_s(gtm_reg.gain_lut, sizeof(uint32_t) * 257, gain_map, sizeof(uint32_t) * 257);
#else
        memcpy(gtm_reg.gain_lut, gain_map, sizeof(uint32_t) * 257);
#endif
        delete[] hist_adapt;
        delete[] hist_equal;
        delete[] hist_equal_db;
        delete[] hist_equal_res;
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
    for(int32_t i=0; i<257; i++)
    {
        gamma_reg.gamma_lut[i] = gamma_22[i];
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

    sensor_crop_reg_calc(g_dng_all_md, reg_ptr->sensor_crop_reg);
    blc_reg_calc(g_dng_all_md, reg_ptr->blc_reg);
    lsc_reg_calc(g_dng_all_md, reg_ptr->lsc_reg);
    ae_stat_reg_cal(g_dng_all_md, reg_ptr->ae_stat_reg);
    awbgain_reg_calc(g_dng_all_md, reg_ptr->awbgain_reg);
    cc_reg_calc(g_dng_all_md, reg_ptr->cc_reg);
    gtm_stat_reg_calc(reg_ptr->gtm_stat_reg);
    gtm_reg_calc(stat_out, reg_ptr->gtm_reg, frame_cnt);
    gamma_reg_calc(reg_ptr->gamma_reg);
    rgb2yuv_reg_calc(reg_ptr->rgb2yuv_reg);
    yuv422_conv_reg_calc(reg_ptr->yuv422_conv_reg);

    hw_base::hw_run(stat_out, frame_cnt);

    log_info("%s run end\n", __FUNCTION__);
}

void fe_firmware::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",     UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s init run end\n", name);
}

fe_firmware::~fe_firmware()
{
    log_info("%s module %s deinit start\n", __FUNCTION__, name);
    log_info("%s module %s deinit end\n", __FUNCTION__, name);
};
