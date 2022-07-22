#include "prophoto2srgb_fw.h"
#include "pipe_register.h"
#include "mat3_3lf.h"

prophoto2srgb_fw::prophoto2srgb_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void prophoto2srgb_fw::fw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                       UINT_32,          &this->bypass}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->fwCfgList.push_back(config[i]);
    }

    log_info("%s init run end\n", name);
}

void prophoto2srgb_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    prophoto2srgb_reg_t* prophoto2srgb_reg = &(regs->prophoto2srgb_reg);

    Mat3_3lf XYZ2sRGB(
        3.2404542, -1.5371385, -0.4985314,
        -0.9692660, 1.8760108, 0.0415560,
        0.0556434, -0.2040259, 1.0572252);
    Mat3_3lf D50_to_D65(
        0.9857398,  0.0000000,  0.0000000,
        0.0000000,  1.0000000,  0.0000000,
        0.0000000,  0.0000000,  1.3194581
        );
    Mat3_3lf FM(global_ref_out->FM);
    Mat3_3lf old_ccm(global_ref_out->ccm);
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

    prophoto2srgb_reg->bypass = 0;
    prophoto2srgb_reg->hlr_en = 1;
    for (int32_t i = 0; i < 9; i++)
    {
        prophoto2srgb_reg->ccm[i] = (int32_t)(ccm.get()[i] * 1024);
    }

    prophoto2srgb_reg->ccm[0] = 1024 - prophoto2srgb_reg->ccm[1] - prophoto2srgb_reg->ccm[2];
    prophoto2srgb_reg->ccm[4] = 1024 - prophoto2srgb_reg->ccm[3] - prophoto2srgb_reg->ccm[5];
    prophoto2srgb_reg->ccm[8] = 1024 - prophoto2srgb_reg->ccm[6] - prophoto2srgb_reg->ccm[7];

    log_1d_array("prophoto2srgb ccm:\n", "%d, ", prophoto2srgb_reg->ccm, 9, 3);
}

prophoto2srgb_fw::~prophoto2srgb_fw()
{
    log_info("%s deinit\n", name);
}