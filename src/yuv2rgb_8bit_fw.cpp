#include "yuv2rgb_8bit_fw.h"
#include "pipe_register.h"

yuv2rgb_8b_fw::yuv2rgb_8b_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void yuv2rgb_8b_fw::fw_init()
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

void yuv2rgb_8b_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    //dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    yuv2rgb_8b_reg_t* yuv2rgb_8b_reg = &(regs->yuv2rgb_8b_reg);

    yuv2rgb_8b_reg->bypass = 0;
    //TODO: yuv2rgb_8b_reg->yuv2rgb_coeff[9]
}

yuv2rgb_8b_fw::~yuv2rgb_8b_fw()
{
    log_info("%s deinit\n", name);
}