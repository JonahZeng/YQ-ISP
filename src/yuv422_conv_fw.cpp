#include "yuv422_conv_fw.h"
#include "pipe_register.h"

yuv422_conv_fw::yuv422_conv_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}
void yuv422_conv_fw::fw_init()
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

void yuv422_conv_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    //dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    yuv422_conv_reg_t* yuv422_conv_reg = &(regs->yuv422_conv_reg);

    yuv422_conv_reg->filter_coeff[0] = 2;
    yuv422_conv_reg->filter_coeff[1] = 6;
    yuv422_conv_reg->filter_coeff[2] = 6;
    yuv422_conv_reg->filter_coeff[3] = 2;
}

yuv422_conv_fw::~yuv422_conv_fw()
{
    log_info("%s deinit\n", name);
}