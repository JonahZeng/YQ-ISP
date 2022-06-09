#pragma once

#include "hw_base.h"
#include "fw_base.h"

class fw_manager:public hw_base
{
public:
    fw_manager() = delete;
    fw_manager(const fw_manager& cp) = delete;
    fw_manager(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) override;
    void hw_init() override;
    ~fw_manager();

    void regsiter_fw_modules(fw_base* fw_md);
    void set_glb_ref(global_ref_out_t* global_ref_out);
    void read_xml_cfg(char* xmlFileName);
private:
    global_ref_out_t* global_ref_out;
    std::vector<fw_base*> fw_list;
    uint32_t bypass;
};