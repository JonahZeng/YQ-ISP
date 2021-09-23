#pragma once

#include "common.h"
#include "statistic_info.h"
#include <vector>
using std::vector;


class hw_base {
public:
    hw_base() = delete;
    hw_base(const hw_base& module) = delete;
    hw_base(uint32_t inpins, uint32_t outpins, const char* inst_name);
    void connect_port(uint32_t out_port, hw_base* next_hw, uint32_t in_port);
    virtual void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) = 0;
    virtual void init() = 0;
    virtual ~hw_base();

    char* name;
    vector<cfgEntry_t> cfgList;
    bool xmlConfigValid;
    bool write_pic;
    vector<uint32_t> write_pic_src_pin;
    char write_pic_format[16];
    char write_pic_path[256];
    uint32_t write_pic_bits;
    
    void reset_hw_cnt_of_outport();
    void release_output_memory();
    
    void write_pic_for_output();

protected:
    uint32_t inpins;
    vector<hw_base*> previous_hw;
    vector<uint32_t> outport_of_previous_hw;

    uint32_t outpins;
    vector<vector<hw_base*>> next_hw_of_outport;
    vector<uint32_t> next_hw_cnt_of_outport;

    vector<data_buffer*> in;
    vector<data_buffer*> out;

public:
    bool prepare_input();
};