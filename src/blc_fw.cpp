#include "blc_fw.h"
#include "pipe_register.h"

blc_fw::blc_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void blc_fw::fw_init()
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

void blc_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out,
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = global_ref_out->meta_data;
    blc_reg_t* blc_reg = &(regs->blc_reg);

    blc_reg->bypass = 0;
    blc_reg->blc_r = all_dng_md->blc_md.r_black_level;
    blc_reg->blc_gr = all_dng_md->blc_md.gr_black_level;
    blc_reg->blc_gb = all_dng_md->blc_md.gb_black_level;
    blc_reg->blc_b = all_dng_md->blc_md.b_black_level;

    blc_reg->normalize_en = 1;
    blc_reg->white_level = (uint32_t)all_dng_md->blc_md.white_level;
}

blc_fw::~blc_fw()
{
    log_info("%s deinit\n", name);
}