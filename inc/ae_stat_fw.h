#pragma once
#include "fw_base.h"

class ae_stat_fw:public fw_base
{
public:
    ae_stat_fw() = delete;
    ae_stat_fw(const ae_stat_fw& cp) = delete;
    ae_stat_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~ae_stat_fw();

    uint32_t bypass;
};