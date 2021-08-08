#pragma once

#include "hw_base.h"
#include <vector>
using std::vector;

class pipeline_manager {
public:
    pipeline_manager();
    ~pipeline_manager();
    void register_module(hw_base* module);
    void init();
    void read_xml_cfg();
    void run(statistic_info_t* stat_out, uint32_t frame_cnt);

public:
    vector<hw_base*> modules;
    uint32_t frames;
    statistic_info_t* stat_addr;
};
