#include "rgb2yuv_fw.h"
#include "fe_firmware.h"

rgb2yuv_fw::rgb2yuv_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void rgb2yuv_fw::fw_init()
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

void rgb2yuv_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    //dng_md_t* all_dng_md = global_ref_out->meta_data;
    rgb2yuv_reg_t* rgb2yuv_reg = &(regs->rgb2yuv_reg);

    rgb2yuv_reg->bypass = 0;
    rgb2yuv_reg->rgb2yuv_coeff[0] = 306;// 0.299⋅R + 0.587⋅G + 0.114 B
    rgb2yuv_reg->rgb2yuv_coeff[1] = 601;
    rgb2yuv_reg->rgb2yuv_coeff[2] = 117;

    rgb2yuv_reg->rgb2yuv_coeff[3] = -173;
    rgb2yuv_reg->rgb2yuv_coeff[4] = -339;
    rgb2yuv_reg->rgb2yuv_coeff[5] = 512;

    rgb2yuv_reg->rgb2yuv_coeff[6] = 512;
    rgb2yuv_reg->rgb2yuv_coeff[7] = -429;
    rgb2yuv_reg->rgb2yuv_coeff[8] = -83;
}

rgb2yuv_fw::~rgb2yuv_fw()
{

}