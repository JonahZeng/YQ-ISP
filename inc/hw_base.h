#pragma once

#include "common.h"
#include "statistic_info.h"
#include <vector>

class hw_base {
public:
    hw_base() = delete;
    hw_base(const hw_base& module) = delete;
    hw_base(uint32_t inpins, uint32_t outpins, const char* inst_name);
    virtual void hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) = 0;
    virtual void hw_init() = 0;
    virtual ~hw_base();

    char* name;
    std::vector<cfgEntry_t> hwCfgList;
    bool xmlConfigValid;
    bool write_pic;
    std::vector<uint32_t> write_pic_src_pin;
    char write_pic_format[16];
    char write_pic_path[256];
    uint32_t write_pic_bits;

    std::string* input_file_name;
    
    void reset_hw_cnt_of_outport();
    void release_output_memory();
    
    void write_pic_for_output();

    uint32_t inpins;
    std::vector<hw_base*> previous_hw;
    std::vector<uint32_t> outport_of_previous_hw;

    uint32_t outpins;
    std::vector<std::vector<hw_base*>> next_hw_of_outport;
    std::vector<uint32_t> next_hw_cnt_of_outport;

protected:
    std::vector<data_buffer*> in;
    std::vector<data_buffer*> out;

public:
    bool prepare_input();
private:
    void write_raw_for_output(FILE* fp);
    void write_pnm_for_output(FILE* fp);
    void write_yuv422_for_output(FILE* fp);
    void write_yuv444_for_output(FILE* fp);
};