#include "fw_base.h"

fw_base::fw_base(uint32_t inpins, uint32_t outpins, const char* inst_name):fwCfgList(0)
{
    this->inpins = inpins;
    this->outpins = outpins;
    size_t namelen = strlen(inst_name);
    if (name != nullptr) {
        if (namelen > 63)
        {
            memcpy(this->name, inst_name, 63);
            this->name[63] = '\0';
        }
        else {
            memcpy(this->name, inst_name, namelen);
            this->name[namelen] = '\0';
        }
    }
}

// void fw_base::fw_init()
// {
//     //must override
// }

// void fw_base::fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, uint32_t frame_cnt, void* pipe_regs)
// {
//     //must override
// }

fw_base::~fw_base()
{
    if (name != nullptr)
    {
        log_info("deinit module %s\n", name);
        delete[] name;
    }
}