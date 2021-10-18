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
    cc* cc_hw = new cc(4, 3, "cc_hw");
    gtm_stat* gtm_stat_hw = new gtm_stat(4, 1, "gtm_stat_hw");
    gtm* gtm_hw = new gtm(4, 3, "gtm_hw");
    gamma* gamma_hw = new gamma(4, 3, "gamma_hw");

    manager->register_module(raw_in);
    manager->register_module(fe_fw);
    manager->register_module(blc_hw);
    manager->register_module(lsc_hw);
    manager->register_module(awbgain_hw);
    manager->register_module(demosaic_hw);
    manager->register_module(cc_hw);
    manager->register_module(gtm_stat_hw);
    manager->register_module(gtm_hw);
    manager->register_module(gamma_hw);

    raw_in->connect_port(0, fe_fw, 0); //raw data
    fe_fw->connect_port(0, blc_hw, 0); //raw data
    fe_fw->connect_port(1, blc_hw, 1);  //regs

    blc_hw->connect_port(0, lsc_hw, 0); //raw data
    fe_fw->connect_port(1, lsc_hw, 1);  //regs

    lsc_hw->connect_port(0, awbgain_hw, 0); //raw data
    fe_fw->connect_port(1, awbgain_hw, 1); //regs

    awbgain_hw->connect_port(0, demosaic_hw, 0); //raw data
    fe_fw->connect_port(1, demosaic_hw, 1); //regs

    demosaic_hw->connect_port(0, cc_hw, 0); //r
    demosaic_hw->connect_port(1, cc_hw, 1); //g
    demosaic_hw->connect_port(2, cc_hw, 2); //b
    fe_fw->connect_port(1, cc_hw, 3); //regs

    cc_hw->connect_port(0, gtm_stat_hw, 0); //r
    cc_hw->connect_port(1, gtm_stat_hw, 1); //g
    cc_hw->connect_port(2, gtm_stat_hw, 2); //b
    fe_fw->connect_port(1, gtm_stat_hw, 3); //regs

    cc_hw->connect_port(0, gtm_hw, 0); //r
    cc_hw->connect_port(1, gtm_hw, 1); //g
    cc_hw->connect_port(2, gtm_hw, 2); //b
    fe_fw->connect_port(1, gtm_hw, 3); //regs

    gtm_hw->connect_port(0, gamma_hw, 0); //r
    gtm_hw->connect_port(1, gamma_hw, 1); //g
    gtm_hw->connect_port(2, gamma_hw, 2); //b
    fe_fw->connect_port(1, gamma_hw, 3); //regs
}