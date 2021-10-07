/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "fileRead.h"
#include "fe_firmware.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    fileRead* raw_in = new fileRead(1, "raw_in");
    fe_firmware* fe_fw = new fe_firmware(1, 2, "fe_fw");
    blc* blc_hw = new blc(2, 1, "blc_hw");
    lsc* lsc_hw = new lsc(2, 1, "lsc_hw");
    awbgain* awbgain_hw = new awbgain(2, 1, "awbgain_hw");
    demosaic* demosaic_hw = new demosaic(2, 3, "demosaic_hw");

    manager->register_module(raw_in);
    manager->register_module(fe_fw);
    manager->register_module(blc_hw);
    manager->register_module(lsc_hw);
    manager->register_module(awbgain_hw);
    manager->register_module(demosaic_hw);

    raw_in->connect_port(0, fe_fw, 0); //raw data
    fe_fw->connect_port(0, blc_hw, 0); //raw data
    fe_fw->connect_port(1, blc_hw, 1);  //regs

    blc_hw->connect_port(0, lsc_hw, 0); //raw data
    fe_fw->connect_port(1, lsc_hw, 1);  //regs

    lsc_hw->connect_port(0, awbgain_hw, 0);//raw data
    fe_fw->connect_port(1, awbgain_hw, 1);//regs

    awbgain_hw->connect_port(0, demosaic_hw, 0);//raw data
    fe_fw->connect_port(1, demosaic_hw, 1);//regs
}