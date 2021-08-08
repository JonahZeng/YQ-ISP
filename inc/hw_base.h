#pragma once

#include "common.h"
#include "statistic_info.h"
#include <vector>
using std::vector;


class hw_base {
public:
    hw_base() = delete;
    hw_base(uint32_t inpins, uint32_t outpins, char* inst_name);
    void connect_port(uint32_t out_port, hw_base* next_hw, uint32_t in_port);
    virtual void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) = 0;
    virtual void init() = 0;
    virtual ~hw_base();

protected:
    char name[64];
    uint32_t inpins;
    vector<hw_base*> previous_hw;
    vector<uint32_t> outport_of_previous_hw;

    uint32_t outpins;
    vector<vector<hw_base*>> next_hw_of_outport;

    vector<data_buffer*> in;
    vector<data_buffer*> out;
    vector<cfgEntry_t> cfgList;
};