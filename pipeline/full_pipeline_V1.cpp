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
    ae_stat* ae_stat_hw = new ae_stat(2, 0, "ae_stat_hw");
    awbgain* awbgain_hw = new awbgain(2, 1, "awbgain_hw");
    demosaic* demosaic_hw = new demosaic(2, 3, "demosaic_hw");
    cc* cc_hw = new cc(4, 3, "cc_hw");
    hsv_lut* hsv_lut_hw = new hsv_lut(4, 3, "hsv_lut_hw");
    prophoto2srgb* prophoto2srgb_hw = new prophoto2srgb(4, 3, "prophoto2srgb_hw");
    gtm_stat* gtm_stat_hw = new gtm_stat(4, 1, "gtm_stat_hw");
    gtm* gtm_hw = new gtm(4, 3, "gtm_hw");
    Gamma* gamma_hw = new Gamma(4, 3, "gamma_hw");
    rgb2yuv* rgb2yuv_hw = new rgb2yuv(4, 3, "rgb2yuv_hw");
    yuv422_conv* yuv422_conv_hw = new yuv422_conv(4, 3, "yuv422_conv_hw");
    yuv_encode* yuv_encode_hw = new yuv_encode(3, 0, "yuv_encode_hw");

    manager->register_hw_module(raw_in);
    manager->register_hw_module(fe_fw);
    manager->register_hw_module(sensor_crop_hw);
    manager->register_hw_module(blc_hw);
    manager->register_hw_module(lsc_hw);
    manager->register_hw_module(ae_stat_hw);
    manager->register_hw_module(awbgain_hw);
    manager->register_hw_module(demosaic_hw);
    manager->register_hw_module(cc_hw);
    manager->register_hw_module(hsv_lut_hw);
    manager->register_hw_module(prophoto2srgb_hw);
    manager->register_hw_module(gtm_stat_hw);
    manager->register_hw_module(gtm_hw);
    manager->register_hw_module(gamma_hw);
    manager->register_hw_module(rgb2yuv_hw);
    manager->register_hw_module(yuv422_conv_hw);
    manager->register_hw_module(yuv_encode_hw);

    manager->connect_port(raw_in, 0, fe_fw, 0); //raw data
    manager->connect_port(fe_fw, 0, sensor_crop_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, sensor_crop_hw, 1);  //regs

    manager->connect_port(sensor_crop_hw, 0, blc_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, blc_hw, 1);  //regs

    manager->connect_port(blc_hw, 0, lsc_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, lsc_hw, 1);  //regs

    manager->connect_port(lsc_hw, 0, ae_stat_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, ae_stat_hw, 1); //regs

    manager->connect_port(lsc_hw, 0, awbgain_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, awbgain_hw, 1); //regs

    manager->connect_port(awbgain_hw, 0, demosaic_hw, 0); //raw data
    manager->connect_port(fe_fw, 1, demosaic_hw, 1); //regs

    manager->connect_port(demosaic_hw, 0, cc_hw, 0); //r
    manager->connect_port(demosaic_hw, 1, cc_hw, 1); //g
    manager->connect_port(demosaic_hw, 2, cc_hw, 2); //b
    manager->connect_port(fe_fw, 1, cc_hw, 3); //regs

    manager->connect_port(cc_hw, 0, gtm_stat_hw, 0); //r
    manager->connect_port(cc_hw, 1, gtm_stat_hw, 1); //g
    manager->connect_port(cc_hw, 2, gtm_stat_hw, 2); //b
    manager->connect_port(fe_fw, 1, gtm_stat_hw, 3); //regs

    manager->connect_port(cc_hw, 0, hsv_lut_hw, 0); //r
    manager->connect_port(cc_hw, 1, hsv_lut_hw, 1); //g
    manager->connect_port(cc_hw, 2, hsv_lut_hw, 2); //b
    manager->connect_port(fe_fw, 1, hsv_lut_hw, 3); //regs

    manager->connect_port(hsv_lut_hw, 0, prophoto2srgb_hw, 0); //r
    manager->connect_port(hsv_lut_hw, 1, prophoto2srgb_hw, 1); //g
    manager->connect_port(hsv_lut_hw, 2, prophoto2srgb_hw, 2); //b
    manager->connect_port(fe_fw, 1, prophoto2srgb_hw, 3); //regs

    manager->connect_port(prophoto2srgb_hw, 0, gtm_hw, 0); //r
    manager->connect_port(prophoto2srgb_hw, 1, gtm_hw, 1); //g
    manager->connect_port(prophoto2srgb_hw, 2, gtm_hw, 2); //b
    manager->connect_port(fe_fw, 1, gtm_hw, 3); //regs

    manager->connect_port(gtm_hw, 0, gamma_hw, 0); //r
    manager->connect_port(gtm_hw, 1, gamma_hw, 1); //g
    manager->connect_port(gtm_hw, 2, gamma_hw, 2); //b
    manager->connect_port(fe_fw, 1, gamma_hw, 3); //regs

    manager->connect_port(gamma_hw, 0, rgb2yuv_hw, 0); //r
    manager->connect_port(gamma_hw, 1, rgb2yuv_hw, 1); //g
    manager->connect_port(gamma_hw, 2, rgb2yuv_hw, 2); //b
    manager->connect_port(fe_fw, 1, rgb2yuv_hw, 3); //regs

    manager->connect_port(rgb2yuv_hw, 0, yuv422_conv_hw, 0); //y
    manager->connect_port(rgb2yuv_hw, 1, yuv422_conv_hw, 1); //u
    manager->connect_port(rgb2yuv_hw, 2, yuv422_conv_hw, 2); //v
    manager->connect_port(fe_fw, 1, yuv422_conv_hw, 3); //regs

    manager->connect_port(yuv422_conv_hw, 0, yuv_encode_hw, 0); //y
    manager->connect_port(yuv422_conv_hw, 1, yuv_encode_hw, 1); //u
    manager->connect_port(yuv422_conv_hw, 2, yuv_encode_hw, 2); //v
}