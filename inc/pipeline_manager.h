﻿#pragma once

#include "hw_base.h"
#include <vector>

class pipeline_manager {
public:
    pipeline_manager();
    ~pipeline_manager();
    void register_hw_module(hw_base* module);
    void init();
    void read_xml_cfg(char* xmlFileName);
    void run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void connect_port(hw_base* pre_hw, uint32_t out_port, hw_base* next_hw, uint32_t in_port);

public:
    std::vector<hw_base*> hw_list;
    uint32_t frames;
    statistic_info_t* stat_addr;
};
