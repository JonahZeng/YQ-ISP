#include "raw_crop_fw.h"
#include "pipe_register.h"

raw_crop_fw::raw_crop_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void raw_crop_fw::fw_init()
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

void raw_crop_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = &(global_ref_out->dng_meta_data);
    raw_crop_reg_t* raw_crop_reg = &(regs->raw_crop_reg);

    raw_crop_reg->bypass = 0;
    raw_crop_reg->origin_x = all_dng_md->sensor_crop_size_info.origin_x;
    raw_crop_reg->origin_y = all_dng_md->sensor_crop_size_info.origin_y;
    raw_crop_reg->width = all_dng_md->sensor_crop_size_info.width;
    raw_crop_reg->height = all_dng_md->sensor_crop_size_info.height;
}

raw_crop_fw::~raw_crop_fw()
{
    log_info("%s deinit\n", name);
}