#pragma once

#include "fw_base.h"

class blc_fw:public fw_base
{
public:
    blc_fw() = delete;
    blc_fw(const blc_fw& cp) = delete;
    blc_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out,
        uint32_t frame_cnt, void* pipe_regs) override;
    ~blc_fw();

    uint32_t bypass;
};