#pragma once
#include "fw_base.h"

class lsc_fw:public fw_base
{
public:
    lsc_fw() = delete;
    lsc_fw(const lsc_fw& cp) = delete;
    lsc_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~lsc_fw();

    uint32_t bypass;
};