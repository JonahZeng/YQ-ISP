#include "fe_firmware.h"
#include "meta_data.h"
#include "chromatix_lsc.h"
#include "dng_tag_values.h"
#include "dng_xy_coord.h"
//#include <sstream>
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

    spdlog::info("focusDist {} focusElement[0].focusDist {} focusElement[5].focusDist {} ",
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
            spdlog::error("can't find neareset two focus dist");
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

    spdlog::info("w1 {} w2 {}", coeff_weight0->weight, coeff_weight1->weight);
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
                spdlog::info("apertureVal input {} apertureVal0 {} apertureVal1 {}", apertureVal, lsc_calib.element[i].apertureVal, lsc_calib.element[i + 1].apertureVal);
                spdlog::info("total_step {} part_step0 {}", total_step, part_step0);
                break;
            }
        }
        if (!flag)
        {
            spdlog::error("can't find two neareset aperture value");
            exit(1);
        }

        get_vignette_coeff_inner(focusDist, &lsc_calib.element[i], coeff_weight, coeff_weight + 1);
        get_vignette_coeff_inner(focusDist, &lsc_calib.element[i + 1], coeff_weight + 2, coeff_weight + 3);

        double weight0 = part_step0 / total_step;
        double weight1 = 1.0 - weight0;

        spdlog::info("weight0 {} weight1 {} weight2 {} weight3 {}",
            (coeff_weight)->weight, (coeff_weight + 1)->weight, (coeff_weight + 2)->weight, (coeff_weight + 3)->weight);

        spdlog::info("weight0 {:01.4f} weight1 {:01.4f}", weight0, weight1);

        (coeff_weight)->weight = (coeff_weight)->weight * weight0;
        (coeff_weight + 1)->weight = (coeff_weight + 1)->weight * weight0;
        (coeff_weight + 2)->weight = (coeff_weight + 2)->weight * weight1;
        (coeff_weight + 3)->weight = (coeff_weight + 3)->weight * weight1;
    }



    spdlog::info("weight0 {} weight1 {} weight2 {} weight3 {}",
        (coeff_weight)->weight, (coeff_weight + 1)->weight, (coeff_weight + 2)->weight, (coeff_weight + 3)->weight);
}

static void lsc_reg_calc(dng_md_t& all_dng_md, lsc_reg_t& lsc_reg)
{
    chromatix_lsc_t lsc_calib =
    {
#include "calib_lsc_sigma_A102_nikon.h"
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

    //std::stringstream ss;

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
            //fprintf(stdout, "%5d, ", lsc_reg.luma_gain[row * LSC_GRID_COLS + col]);
            //ss << lsc_reg.luma_gain[row * LSC_GRID_COLS + col] << ", ";
        }
        //ss << std::endl;
        //fprintf(stdout, "\n");
    }
    //spdlog::info("\n{}", ss.str());
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
    spdlog::info("compensat_gain {:.4f}", compensat_gain);

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
        spdlog::info("========input xy = {:.4f} {:.4f}========", input_x, input_y);
        std::cout << "cameraNeutral:" << cameraNeutral << std::endl;

        dist_1_x = abs(input_x - white_point1_xy[0]);
        dist_1_y = abs(input_y - white_point1_xy[1]);

        dist_1 = sqrt(dist_1_x * dist_1_x + dist_1_y * dist_1_y);

        dist_2_x = abs(input_x - white_point2_xy[0]);
        dist_2_y = abs(input_y - white_point2_xy[1]);

        dist_2 = sqrt(dist_2_x * dist_2_x + dist_2_y * dist_2_y);
        spdlog::info("dist to 1 = {:.4f}, to 2 = {:.4f}", dist_1, dist_2);

        weight_1 = dist_1_x / (dist_1_x + dist_2_x);
        weight_2 = dist_2_x / (dist_1_x + dist_2_x);


        spdlog::info("weight1 = {:.4f}, weight2 = {:.4f}", weight_1, weight_2);

        cv::Mat tmp_CM = CM_2 * weight_2 + CM_1 * weight_1;
        tmp_CM = tmp_CM.inv();

        std::cout << "tmp_CM:" << tmp_CM << std::endl;

        cv::Mat tmp_XYZ = tmp_CM * cameraNeutral;
        std::cout << "tmp_XYZ:" << tmp_XYZ << std::endl;

        double X = tmp_XYZ.at<double>(0);
        double Y = tmp_XYZ.at<double>(1);
        double Z = tmp_XYZ.at<double>(2);

        out_x = X / (X + Y + Z);
        out_y = Y / (X + Y + Z);

        spdlog::info("========out xy = {:.4f} {:.4f}========", out_x, out_y);

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
    cv::Mat cameraNeutral = (cv::Mat_<double>(3, 1) << all_dng_md.awb_md.r_Neutral, all_dng_md.awb_md.g_Neutral, all_dng_md.awb_md.b_Neutral);

    if (all_dng_md.cc_md.Analogbalance[0] != 1.0 || all_dng_md.cc_md.Analogbalance[1] != 1.0 || all_dng_md.cc_md.Analogbalance[2] != 1.0)
    {
        spdlog::error("AB is not all == 1.0");
    }
    if (all_dng_md.cc_md.CameraCalibration1[0][0] != 1.0 || all_dng_md.cc_md.CameraCalibration1[1][1] != 1.0
        || all_dng_md.cc_md.CameraCalibration1[2][2] != 1.0)
    {
        spdlog::error("CC1 is not I matrix");
    }
    if (all_dng_md.cc_md.CameraCalibration2[0][0] != 1.0 || all_dng_md.cc_md.CameraCalibration2[1][1] != 1.0
        || all_dng_md.cc_md.CameraCalibration2[2][2] != 1.0)
    {
        spdlog::error("CC2 is not I matrix");
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
    std::cout << "camera2XYZ_mat" << FM << std::endl;

    //cv::Mat XYZ2sRGB = (cv::Mat_<double>(3, 3) <<
    //    3.2404542, -1.5371385, -0.4985314,
    //    -0.9692660, 1.8760108, 0.0415560,
    //    0.0556434, -0.2040259, 1.0572252);
    cv::Mat XYZ2photoRGB = (cv::Mat_<double>(3, 3) <<
        1.3459433, -0.2556075, -0.0511118,
        -0.5445989, 1.5081673, 0.0205351,
        0.0000000, 0.0000000, 1.2118128);
    //std::cout << "XYZ2sRGB:" << XYZ2sRGB << std::endl;
    std::cout << "ccm:" << std::endl;
    cv::Mat ccm = XYZ2photoRGB * FM;
    std::cout << ccm << std::endl;

    cc_reg.bypass = 0;
    for (int32_t i = 0; i < 9; i++)
    {
        cc_reg.ccm[i] = (int32_t)(ccm.at<double>(i / 3, i % 3) * 1024);
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
        memcpy(gtm_reg.tone_curve, tone_curve_y, sizeof(uint32_t) * 257);
#endif
    }
    else 
    {
        const uint32_t* luma_hist = stat_out->gtm_stat.luma_hist;
        const uint32_t total_cnt = stat_out->gtm_stat.total_pixs;
        uint32_t* hist_equal = new uint32_t[256];
        double* hist_equal_db = new double[256 + 8]; //savgol window size=9, order=3
        double* hist_equal_res = new double[256];

        hist_equal[0] = luma_hist[0];
        for (int32_t i = 1; i < 256; i++)
        {
            hist_equal[i] = hist_equal[i - 1] + luma_hist[i];
        }
        assert(hist_equal[255] == total_cnt);
        for (int32_t i = 0; i < 256; i++)
        {
            hist_equal_db[i + 4] = double(hist_equal[i]) * 255.0 / total_cnt;
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

        uint32_t gain_map[257] = {
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

        for (int32_t i = 0; i < 256; i++)
        {
            hist_equal_res[i] = hist_equal_res[i] * 1024 / i;
            gain_map[i] = uint32_t(hist_equal_res[i]);
            printf("%d, ", gain_map[i]);
            if (i % 32 == 31)
            {
                printf("\n");
            }
        }
        gain_map[256] = gain_map[255];
#ifdef _MSC_VER
        memcpy_s(gtm_reg.gain_lut, sizeof(uint32_t) * 257, gain_map, sizeof(uint32_t) * 257);
#else
        memcpy(gtm_reg.tone_curve, tone_curve_y, sizeof(uint32_t) * 257);
#endif
        delete[] hist_equal;
        delete[] hist_equal_db;
        delete[] hist_equal_res;
    }

    //TODO: GTM stat calc gtm curve
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
    gamma_reg.gamma_x1024 = 2253;
}

void fe_firmware::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);

    data_buffer* input = in[0];
    data_buffer* output0 = new data_buffer(input->width, input->height, input->data_type, input->bayer_pattern, "fe_fw_out0");

    memcpy(output0->data_ptr, input->data_ptr, input->width*input->height * sizeof(uint16_t));

    size_t reg_len = sizeof(fe_module_reg_t) / sizeof(uint16_t);
    if (reg_len * sizeof(uint16_t) < sizeof(fe_module_reg_t))
    {
        reg_len += 1;
    }
    data_buffer* output1 = new data_buffer((uint32_t)reg_len, 1, RAW, RGGB, "fe_fw_out1");
    fe_module_reg_t* reg_ptr = (fe_module_reg_t*)output1->data_ptr;

    out[0] = output0;
    out[1] = output1;

    blc_reg_calc(g_dng_all_md, reg_ptr->blc_reg);
    lsc_reg_calc(g_dng_all_md, reg_ptr->lsc_reg);
    awbgain_reg_calc(g_dng_all_md, reg_ptr->awbgain_reg);
    cc_reg_calc(g_dng_all_md, reg_ptr->cc_reg);
    gtm_stat_reg_calc(reg_ptr->gtm_stat_reg);
    gtm_reg_calc(stat_out, reg_ptr->gtm_reg, frame_cnt);
    gamma_reg_calc(reg_ptr->gamma_reg);

    hw_base::hw_run(stat_out, frame_cnt);

    spdlog::info("{0} run end", __FUNCTION__);
}

void fe_firmware::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",     UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}

fe_firmware::~fe_firmware()
{
    spdlog::info("{0} module {1} deinit start", __FUNCTION__, name);
    spdlog::info("{0} module {1} deinit end", __FUNCTION__, name);
};