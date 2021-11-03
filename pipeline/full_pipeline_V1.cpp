/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "fileRead.h"
#include "fe_firmware.h"
#include "yuv_encode.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    fileRead* raw_in = new fileRead(1, "raw_in");
    fe_firmware* fe_fw = new fe_firmware(1, 2, "fe_fw");
    sensor_crop* sensor_crop_hw = new sensor_crop(2, 1, "sensor_crop_hw");
    blc* blc_hw = new blc(2, 1, "blc_hw");
    lsc* lsc_hw = new lsc(2, 1, "lsc_hw");
    awbgain* awbgain_hw = new awbgain(2, 1, "awbgain_hw");
    demosaic* demosaic_hw = new demosaic(2, 3, "demosaic_hw");
    cc* cc_hw = new cc(4, 3, "cc_hw");
    gtm_stat* gtm_stat_hw = new gtm_stat(4, 1, "gtm_stat_hw");
    gtm* gtm_hw = new gtm(4, 3, "gtm_hw");
    Gamma* gamma_hw = new Gamma(4, 3, "gamma_hw");
    rgb2yuv* rgb2yuv_hw = new rgb2yuv(4, 3, "rgb2yuv_hw");
    yuv422_conv* yuv422_conv_hw = new yuv422_conv(4, 3, "yuv422_conv_hw");
    yuv_encode* yuv_encode_hw = new yuv_encode(3, 0, "yuv_encode_hw");

    manager->register_module(raw_in);
    manager->register_module(fe_fw);
    manager->register_module(sensor_crop_hw);
    manager->register_module(blc_hw);
    manager->register_module(lsc_hw);
    manager->register_module(awbgain_hw);
    manager->register_module(demosaic_hw);
    manager->register_module(cc_hw);
    manager->register_module(gtm_stat_hw);
    manager->register_module(gtm_hw);
    manager->register_module(gamma_hw);
    manager->register_module(rgb2yuv_hw);
    manager->register_module(yuv422_conv_hw);
    manager->register_module(yuv_encode_hw);

    raw_in->connect_port(0, fe_fw, 0); //raw data
    fe_fw->connect_port(0, sensor_crop_hw, 0); //raw data
    fe_fw->connect_port(1, sensor_crop_hw, 1);  //regs

    sensor_crop_hw->connect_port(0, blc_hw, 0); //raw data
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

    gamma_hw->connect_port(0, rgb2yuv_hw, 0); //r
    gamma_hw->connect_port(1, rgb2yuv_hw, 1); //g
    gamma_hw->connect_port(2, rgb2yuv_hw, 2); //b
    fe_fw->connect_port(1, rgb2yuv_hw, 3); //regs

    rgb2yuv_hw->connect_port(0, yuv422_conv_hw, 0); //y
    rgb2yuv_hw->connect_port(1, yuv422_conv_hw, 1); //u
    rgb2yuv_hw->connect_port(2, yuv422_conv_hw, 2); //v
    fe_fw->connect_port(1, yuv422_conv_hw, 3); //regs

    yuv422_conv_hw->connect_port(0, yuv_encode_hw, 0); //y
    yuv422_conv_hw->connect_port(1, yuv_encode_hw, 1); //u
    yuv422_conv_hw->connect_port(2, yuv_encode_hw, 2); //v
}