#pragma once
#include "fw_base.h"

class gtm_fw:public fw_base
{
public:
    gtm_fw() = delete;
    gtm_fw(const gtm_fw& cp) = delete;
    gtm_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~gtm_fw();

    uint32_t bypass;
};