/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "fileRead.h"
#include "fe_firmware.h"
#include "yuv_encode_hw.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    fileRead* raw_in = new fileRead(1, "raw_in");
    fe_firmware* fe_fw = new fe_firmware(1, 2, "fe_fw");
    raw_crop_hw* raw_crop_hw_inst = new raw_crop_hw(2, 1, "raw_crop_hw");
    blc_hw* blc_hw_inst = new blc_hw(2, 1, "blc_hw");
    lsc_hw* lsc_hw_inst = new lsc_hw(2, 1, "lsc_hw");
    ae_stat_hw* ae_stat_hw_inst = new ae_stat_hw(2, 0, "ae_stat_hw");
    awbgain_hw* awbgain_hw_inst = new awbgain_hw(2, 1, "awbgain_hw");
    demosaic_hw* demosaic_hw_inst = new demosaic_hw(2, 3, "demosaic_hw");
    cc_hw* cc_hw_inst = new cc_hw(4, 3, "cc_hw");
    hsv_lut_hw* hsv_lut_hw_inst = new hsv_lut_hw(4, 3, "hsv_lut_hw");
    prophoto2srgb_hw* prophoto2srgb_hw_inst = new prophoto2srgb_hw(4, 3, "prophoto2srgb_hw");
    gtm_stat_hw* gtm_stat_hw_inst = new gtm_stat_hw(4, 1, "gtm_stat_hw");
    gtm_hw* gtm_hw_inst = new gtm_hw(4, 3, "gtm_hw");
    Gamma_hw* gamma_hw_inst = new Gamma_hw(4, 3, "gamma_hw");
    rgb2yuv_hw* rgb2yuv_hw_inst = new rgb2yuv_hw(4, 3, "rgb2yuv_hw");
    yuv422_conv_hw* yuv422_conv_hw_inst = new yuv422_conv_hw(4, 3, "yuv422_conv_hw");
    yuv_encode_hw* yuv_encode_hw_inst = new yuv_encode_hw(3, 0, "yuv_encode_hw");

    manager->register_hw_module(raw_in);
    manager->register_hw_module(fe_fw);
    manager->register_hw_module(raw_crop_hw_inst);
    manager->register_hw_module(blc_hw_inst);
    manager->register_hw_module(lsc_hw_inst);
    manager->register_hw_module(ae_stat_hw_inst);
    manager->register_hw_module(awbgain_hw_inst);
    manager->register_hw_module(demosaic_hw_inst);
    manager->register_hw_module(cc_hw_inst);
    manager->register_hw_module(hsv_lut_hw_inst);
    manager->register_hw_module(prophoto2srgb_hw_inst);
    manager->register_hw_module(gtm_stat_hw_inst);
    manager->register_hw_module(gtm_hw_inst);
    manager->register_hw_module(gamma_hw_inst);
    manager->register_hw_module(rgb2yuv_hw_inst);
    manager->register_hw_module(yuv422_conv_hw_inst);
    manager->register_hw_module(yuv_encode_hw_inst);

    manager->connect_port(raw_in, 0, fe_fw, 0); //raw data
    manager->connect_port(fe_fw, 0, raw_crop_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, raw_crop_hw_inst, 1);  //regs

    manager->connect_port(raw_crop_hw_inst, 0, blc_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, blc_hw_inst, 1);  //regs

    manager->connect_port(blc_hw_inst, 0, lsc_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, lsc_hw_inst, 1);  //regs

    manager->connect_port(lsc_hw_inst, 0, ae_stat_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, ae_stat_hw_inst, 1); //regs

    manager->connect_port(lsc_hw_inst, 0, awbgain_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, awbgain_hw_inst, 1); //regs

    manager->connect_port(awbgain_hw_inst, 0, demosaic_hw_inst, 0); //raw data
    manager->connect_port(fe_fw, 1, demosaic_hw_inst, 1); //regs

    manager->connect_port(demosaic_hw_inst, 0, cc_hw_inst, 0); //r
    manager->connect_port(demosaic_hw_inst, 1, cc_hw_inst, 1); //g
    manager->connect_port(demosaic_hw_inst, 2, cc_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, cc_hw_inst, 3); //regs

    manager->connect_port(cc_hw_inst, 0, gtm_stat_hw_inst, 0); //r
    manager->connect_port(cc_hw_inst, 1, gtm_stat_hw_inst, 1); //g
    manager->connect_port(cc_hw_inst, 2, gtm_stat_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, gtm_stat_hw_inst, 3); //regs

    manager->connect_port(cc_hw_inst, 0, hsv_lut_hw_inst, 0); //r
    manager->connect_port(cc_hw_inst, 1, hsv_lut_hw_inst, 1); //g
    manager->connect_port(cc_hw_inst, 2, hsv_lut_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, hsv_lut_hw_inst, 3); //regs

    manager->connect_port(hsv_lut_hw_inst, 0, prophoto2srgb_hw_inst, 0); //r
    manager->connect_port(hsv_lut_hw_inst, 1, prophoto2srgb_hw_inst, 1); //g
    manager->connect_port(hsv_lut_hw_inst, 2, prophoto2srgb_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, prophoto2srgb_hw_inst, 3); //regs

    manager->connect_port(prophoto2srgb_hw_inst, 0, gtm_hw_inst, 0); //r
    manager->connect_port(prophoto2srgb_hw_inst, 1, gtm_hw_inst, 1); //g
    manager->connect_port(prophoto2srgb_hw_inst, 2, gtm_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, gtm_hw_inst, 3); //regs

    manager->connect_port(gtm_hw_inst, 0, gamma_hw_inst, 0); //r
    manager->connect_port(gtm_hw_inst, 1, gamma_hw_inst, 1); //g
    manager->connect_port(gtm_hw_inst, 2, gamma_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, gamma_hw_inst, 3); //regs

    manager->connect_port(gamma_hw_inst, 0, rgb2yuv_hw_inst, 0); //r
    manager->connect_port(gamma_hw_inst, 1, rgb2yuv_hw_inst, 1); //g
    manager->connect_port(gamma_hw_inst, 2, rgb2yuv_hw_inst, 2); //b
    manager->connect_port(fe_fw, 1, rgb2yuv_hw_inst, 3); //regs

    manager->connect_port(rgb2yuv_hw_inst, 0, yuv422_conv_hw_inst, 0); //y
    manager->connect_port(rgb2yuv_hw_inst, 1, yuv422_conv_hw_inst, 1); //u
    manager->connect_port(rgb2yuv_hw_inst, 2, yuv422_conv_hw_inst, 2); //v
    manager->connect_port(fe_fw, 1, yuv422_conv_hw_inst, 3); //regs

    manager->connect_port(yuv422_conv_hw_inst, 0, yuv_encode_hw_inst, 0); //y
    manager->connect_port(yuv422_conv_hw_inst, 1, yuv_encode_hw_inst, 1); //u
    manager->connect_port(yuv422_conv_hw_inst, 2, yuv_encode_hw_inst, 2); //v

    //TODO: fw manager
    //TODO: algo module fw implement
}