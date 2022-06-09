#pragma once
#include "fw_base.h"

class awbgain_fw:public fw_base
{
public:
    awbgain_fw() = delete;
    awbgain_fw(const awbgain_fw& cp) = delete;
    awbgain_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~awbgain_fw();

    uint32_t bypass;
};