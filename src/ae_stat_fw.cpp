#include "ae_stat_fw.h"
#include "fe_firmware.h"

ae_stat_fw::ae_stat_fw(uint32_t inpins, uint32_t outpins, const char* inst_name):fw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

void ae_stat_fw::fw_init()
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

void ae_stat_fw::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
    uint32_t frame_cnt, void* pipe_regs)
{
    fe_module_reg_t* regs = (fe_module_reg_t*)pipe_regs;
    dng_md_t* all_dng_md = global_ref_out->meta_data;
    ae_stat_reg_t* ae_stat_reg = &(regs->ae_stat_reg);

    ae_stat_reg->bypass = 0;
    ae_stat_reg->skip_step_x = 2;
    ae_stat_reg->skip_step_y = 2;
    ae_stat_reg->block_width = all_dng_md->sensor_crop_size_info.width / 32;
    if (ae_stat_reg->block_width % 2 == 1)
    {
        ae_stat_reg->block_width -= 1;
    }
    ae_stat_reg->block_height = all_dng_md->sensor_crop_size_info.height / 32;
    if (ae_stat_reg->block_height % 2 == 1)
    {
        ae_stat_reg->block_height -= 1;
    }

    double step_x_lf = double(all_dng_md->sensor_crop_size_info.width) / 32.0;
    double step_y_lf = double(all_dng_md->sensor_crop_size_info.height) / 32.0;
    for (int32_t i = 0; i < 32; i++)
    {
        double tmp_x = step_x_lf * i;
        tmp_x = round(tmp_x);
        uint32_t x_ = uint32_t(tmp_x);
        if (x_ % 2 == 1)
        {
            x_ += 1;
        }
        ae_stat_reg->start_x[i] = x_;
        double tmp_y = step_y_lf * i;
        tmp_y = round(tmp_y);
        uint32_t y_ = uint32_t(tmp_y);
        if (y_ % 2 == 1)
        {
            y_ += 1;
        }
        ae_stat_reg->start_y[i] = y_;
    }
}

ae_stat_fw::~ae_stat_fw()
{

}