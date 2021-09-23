/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "fileRead.h"
#include "blc.h"
#include "fe_firmware.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    fileRead* raw_in = new fileRead(1, "raw_in");
    fe_firmware* fe_fw = new fe_firmware(1, 2, "fe_fw");
    blc* blc_hw = new blc(2, 1, "blc_hw");

    manager->register_module(raw_in);
    manager->register_module(blc_hw);
    manager->register_module(fe_fw);

    raw_in->connect_port(0, fe_fw, 0); //raw data
    fe_fw->connect_port(0, blc_hw, 0); //raw data
    fe_fw->connect_port(1, blc_hw, 1);  //regs
}


