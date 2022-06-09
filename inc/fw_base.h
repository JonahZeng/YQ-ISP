#pragma once

#include "common.h"
#include "statistic_info.h"
#include <cstdint>
#include <vector>
#include <string>

class fw_base{
public:
    fw_base() = delete;
    fw_base(const fw_base& cp) = delete;
    fw_base(uint32_t inpins, uint32_t outpins, const char* inst_name);
    virtual void fw_init() = 0;
    virtual void fw_exec(statistic_info_t* stat_in, global_ref_out_t* global_ref_out, 
        uint32_t frame_cnt, void* pipe_regs) = 0;
    virtual ~fw_base();

    uint32_t inpins;
    uint32_t outpins;
    char* name;
    std::vector<cfgEntry_t> fwCfgList;
};