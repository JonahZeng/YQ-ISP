#include "cc_fw.h"
#include "fe_firmware.h"
#include "chromatix_cc.h"
#include "dng_tag_values.h"
#include "dng_xy_coord.h"
#include "mat3_3lf.h"
#include <assert.h>

cc_fw::cc_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void cc_fw::fw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                       UINT_32,          &this->bypass                }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->fwCfgList.push_back(config[i]);
    }

    log_info("%s init run end\n", name);
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

void cc_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = global_ref_out->meta_data;
    cc_reg_t* cc_reg = &(regs->cc_reg);

    chromatix_cc_t cc_calib = {
#include "calib_cc_sigma_A102_nikonD610.h"
    };
    if (cc_calib.cc_mode == 1)
    {
        Vector3lf cameraNeutral = { {all_dng_md->awb_md.r_Neutral, all_dng_md->awb_md.g_Neutral, all_dng_md->awb_md.b_Neutral} };

        if (all_dng_md->cc_md.Analogbalance[0] != 1.0 || all_dng_md->cc_md.Analogbalance[1] != 1.0 || all_dng_md->cc_md.Analogbalance[2] != 1.0)
        {
            log_error("AB is not all == 1.0\n");
        }
        if (all_dng_md->cc_md.CameraCalibration1[0][0] != 1.0 || all_dng_md->cc_md.CameraCalibration1[1][1] != 1.0
            || all_dng_md->cc_md.CameraCalibration1[2][2] != 1.0)
        {
            log_error("CC1 is not I matrix\n");
        }
        if (all_dng_md->cc_md.CameraCalibration2[0][0] != 1.0 || all_dng_md->cc_md.CameraCalibration2[1][1] != 1.0
            || all_dng_md->cc_md.CameraCalibration2[2][2] != 1.0)
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
                CM_1.get()[row * 3 + col] = all_dng_md->cc_md.ColorMatrix1[row][col];
                CM_2.get()[row * 3 + col] = all_dng_md->cc_md.ColorMatrix2[row][col];

                FM_1.get()[row * 3 + col] = all_dng_md->cc_md.ForwardMatrix1[row][col];
                FM_2.get()[row * 3 + col] = all_dng_md->cc_md.ForwardMatrix1[row][col];
            }
        }

        uint32_t CalibrationIlluminant1 = all_dng_md->cc_md.CalibrationIlluminant1;
        uint32_t CalibrationIlluminant2 = all_dng_md->cc_md.CalibrationIlluminant2;
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

        global_ref_out->wp_x = x;
        global_ref_out->wp_y = y;

        Mat3_3lf FM = FM_1 * weight1 + FM_2 * weight2;
        log_array("camera2XYZ_mat\n", "%lf, ", FM.get(), 9, 3);

        for (int32_t i = 0; i < 9; i++)
        {
            global_ref_out->FM[i] = FM.get()[i];
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

        cc_reg->bypass = 0;
        for (int32_t i = 0; i < 9; i++)
        {
            cc_reg->ccm[i] = (int32_t)(ccm.get()[i] * 1024);
            global_ref_out->ccm[i] = ccm.get()[i];
        }

        cc_reg->ccm[0] = 1024 - cc_reg->ccm[1] - cc_reg->ccm[2];
        cc_reg->ccm[4] = 1024 - cc_reg->ccm[3] - cc_reg->ccm[5];
        cc_reg->ccm[8] = 1024 - cc_reg->ccm[6] - cc_reg->ccm[7];

        log_array("final ccm:\n", "%d, ", cc_reg->ccm, 9, 3);
    }
}

cc_fw::~cc_fw()
{

}