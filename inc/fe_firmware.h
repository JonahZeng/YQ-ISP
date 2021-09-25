#pragma once

#include "blc.h"

typedef struct fe_module_reg_s {
    blc_reg_t blc_reg;
}fe_module_reg_t;

class fe_firmware :public hw_base {
public:
    fe_firmware() = delete;
    fe_firmware(const fe_firmware& cp) = delete;
    fe_firmware(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~fe_firmware();
private:
    uint32_t bypass;
};