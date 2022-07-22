#pragma once

#include "common.h"
#include "statistic_info.h"
#include <vector>

class pipeline_manager;

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
    bool write_pic0;
    std::vector<uint32_t> write_pic0_src_pin;
    char write_pic0_format[16];
    char write_pic0_path[256];
    uint32_t write_pic0_bits;

    bool write_pic1;
    std::vector<uint32_t> write_pic1_src_pin;
    char write_pic1_format[16];
    char write_pic1_path[256];
    uint32_t write_pic1_bits;

    std::string* input_file_name;
    
    void reset_hw_cnt_of_outport();
    void release_output_memory();
    
    void write_pic0_for_output();
    void write_pic1_for_output();
protected:
    const uint32_t inpins;
    const uint32_t outpins;
private:

    std::vector<hw_base*> previous_hw;
    std::vector<uint32_t> outport_of_previous_hw;

    std::vector<std::vector<hw_base*>> next_hw_of_outport;
    std::vector<uint32_t> next_hw_cnt_of_outport;

protected:
    std::vector<data_buffer*> in;
    std::vector<data_buffer*> out;

public:
    bool prepare_input();
private:
    void write_raw_for_output(FILE* fp, int32_t pic_no);
    void write_pnm_for_output(FILE* fp, int32_t pic_no);
    void write_yuv420_for_output(FILE* fp, int32_t pic_no); //NV12
    void write_yuv422_for_output(FILE* fp, int32_t pic_no); //UYVY
    void write_yuv444_for_output(FILE* fp, int32_t pic_no); //YUVYUV interleaved
    friend class pipeline_manager;
};