#include "hsv_lut_fw.h"
#include "pipe_register.h"
#include "dng_tag_values.h"
#include "dng_xy_coord.h"

hsv_lut_fw::hsv_lut_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void hsv_lut_fw::fw_init()
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

void hsv_lut_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    hsv_lut_reg_t* hsv_lut_reg = &(regs->hsv_lut_reg);

    hsv_lut_reg->bypass = 0;
    hsv_lut_reg->twoD_enable = all_dng_md->hsv_lut_md.twoD_enable;
    hsv_lut_reg->threeD_enable = all_dng_md->hsv_lut_md.threeD_enable;
    hsv_lut_reg->twoD_map_encoding = all_dng_md->hsv_lut_md.fHueSatMapEncoding;
    hsv_lut_reg->threeD_map_encoding = all_dng_md->hsv_lut_md.fLookTableEncoding;

    uint32_t light1 = all_dng_md->hsv_lut_md.fCalibrationIlluminant1;
    uint32_t light2 = all_dng_md->hsv_lut_md.fCalibrationIlluminant2;
    dng_xy_coord wp1, wp2;
    get_xy_wp(light1, light2, wp1, wp2);

    double x1 = wp1.x - global_ref_out->wp_x;
    double y1 = wp1.y - global_ref_out->wp_y;
    double dist1 = sqrt(x1 * x1 + y1 * y1);
    double x2 = wp2.x - global_ref_out->wp_x;
    double y2 = wp2.y - global_ref_out->wp_y;
    double dist2 = sqrt(x2 * x2 + y2 * y2);

    double weight = dist1 / (dist1 + dist2);

    dng_hue_sat_map* result = dng_hue_sat_map::Interpolate(all_dng_md->hsv_lut_md.fHueSatDeltas1, all_dng_md->hsv_lut_md.fHueSatDeltas2, weight);
    if (result->IsValid())
    {
        uint32_t hD=0, sD=0, vD=0;
        result->GetDivisions(hD, sD, vD);
        hsv_lut_reg->twoDHueDivisions = hD;
        hsv_lut_reg->twoDSatDivisions = sD;

        memcpy(hsv_lut_reg->twoDmap, result->GetConstDeltas(), sizeof(dng_hue_sat_map::HSBModify) * hD * sD * vD);
    }
    delete result;

    uint32_t hD = 0, sD = 0, vD = 0;
    all_dng_md->hsv_lut_md.fLookTable.GetDivisions(hD, sD, vD);
    hsv_lut_reg->threeDHueDivisions = hD;
    hsv_lut_reg->threeDSatDivisions = sD;
    hsv_lut_reg->threeDValDivisions = vD;

    memcpy(hsv_lut_reg->threeDmap, all_dng_md->hsv_lut_md.fLookTable.GetConstDeltas(), sizeof(dng_hue_sat_map::HSBModify) * hD * sD * vD);
}

hsv_lut_fw::~hsv_lut_fw()
{
    log_info("%s deinit\n", name);
}