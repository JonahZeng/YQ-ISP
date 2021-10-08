#pragma once

#include "hw_base.h"
#include <stdint.h>

typedef struct cc_reg_s
{
    uint32_t bypass;
    int32_t ccm[9];
}cc_reg_t;

class cc :public hw_base {
public:
    cc() = delete;
    cc(const cc& cp) = delete;
    cc(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void init();
    ~cc();

private:
    void checkparameters(cc_reg_t* reg);
    uint32_t bypass;
    vector<int32_t> ccm;
};
