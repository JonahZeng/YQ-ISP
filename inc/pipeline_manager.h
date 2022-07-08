#pragma once

#include "hw_base.h"
#include <vector>

class pipeline_manager {
public:
    pipeline_manager();
    ~pipeline_manager();
    void register_hw_module(hw_base* module);
    void init();
    void read_xml_cfg();
    void run(statistic_info_t* stat_out, uint32_t frame_cnt);
    void connect_port(hw_base* pre_hw, uint32_t out_port, hw_base* next_hw, uint32_t in_port);

public:
    std::vector<hw_base*> hw_list;
    uint32_t frames;
    statistic_info_t* stat_addr;
    global_ref_out_t* global_ref_out;
    std::string cfg_file_name;

private:
    static pipeline_manager* g_pipe_manager;
public:
    static pipeline_manager* get_current_pipe_manager();
    static void set_current_pipe_manager(pipeline_manager* pipe_manager);
};
