/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "file_read.h"
#include "awb_gain_hw.h"


void test_awbgain_pipeline(pipeline_manager* manager)
{
    file_read* raw_in = new file_read(1, "raw_in");
    awbgain_hw* awbgain_hw_inst = new awbgain_hw(1, 1, "awbgain_hw");

    manager->register_hw_module(raw_in);
    manager->register_hw_module(awbgain_hw_inst);

    manager->connect_port(raw_in, 0, awbgain_hw_inst, 0); //raw data
}