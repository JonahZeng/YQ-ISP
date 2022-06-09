#pragma once
#include "fw_base.h"

class yuv2rgb_8b_fw:public fw_base
{
public:
    yuv2rgb_8b_fw() = delete;
    yuv2rgb_8b_fw(const yuv2rgb_8b_fw& cp) = delete;
    yuv2rgb_8b_fw(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void fw_init() override;
    void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) override;
    ~yuv2rgb_8b_fw();

    uint32_t bypass;
};