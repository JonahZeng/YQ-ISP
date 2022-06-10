#include "awb_gain_fw.h"
#include "pipe_register.h"

awbgain_fw::awbgain_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void awbgain_fw::fw_init()
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

void awbgain_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = global_ref_out->meta_data;
    awbgain_reg_t* awbgain_reg = &(regs->awbgain_reg);

    double r_gain = 1.0 / all_dng_md->awb_md.r_Neutral;
    double g_gain = 1.0 / all_dng_md->awb_md.g_Neutral;
    double b_gain = 1.0 / all_dng_md->awb_md.b_Neutral;

    awbgain_reg->bypass = 0;
    awbgain_reg->r_gain = uint32_t(r_gain * 1024);
    awbgain_reg->gr_gain = uint32_t(g_gain * 1024);
    awbgain_reg->gb_gain = awbgain_reg->gr_gain;
    awbgain_reg->b_gain = uint32_t(b_gain * 1024);

    double BaselineExposure = all_dng_md->ae_comp_md.BaselineExposure;
    double compensat_gain = pow(2, BaselineExposure);
    log_info("compensat_gain %lf, disable baseline exposure compensation\n", compensat_gain);

    awbgain_reg->ae_compensat_gain = 1024;// (uint32_t)(compensat_gain * 1024);
}

awbgain_fw::~awbgain_fw()
{
    log_info("%s deinit\n", name);
}