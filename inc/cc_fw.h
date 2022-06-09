#pragma once
#include "fw_base.h"


class cc_fw:public fw_base
{
public:
    cc_fw() = delete;
    cc_fw(const cc_fw& cp) = delete;
    cc_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~cc_fw();

    uint32_t bypass;
};