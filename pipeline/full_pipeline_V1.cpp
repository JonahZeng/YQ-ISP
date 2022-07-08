/*
pipeline construction
*/
#include "pipeline_manager.h"
#include "file_read.h"
#include "pipe_register.h"
#include "yuv_encode_hw.h"
#include "fw_manager.h"
#include "raw_crop_fw.h"
#include "blc_fw.h"
#include "lsc_fw.h"
#include "ae_stat_fw.h"
#include "awb_gain_fw.h"
#include "demosaic_fw.h"
#include "cc_fw.h"
#include "hsv_lut_fw.h"
#include "prophoto2srgb_fw.h"
#include "gtm_stat_fw.h"
#include "gtm_fw.h"
#include "gamma_fw.h"
#include "rgb2yuv_fw.h"
#include "yuv422_conv_fw.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    file_read* raw_in = new file_read(1, "raw_in");
    fw_manager* fe_fw_manager = new fw_manager(1, 2, "fe_fw_manager");
    raw_crop_hw* raw_crop_hw_inst = new raw_crop_hw(2, 1, "raw_crop_hw");
    raw_crop_fw* raw_crop_fw_inst = new raw_crop_fw(1, 1, "raw_crop_fw");
    blc_hw* blc_hw_inst = new blc_hw(2, 1, "blc_hw");
    blc_fw* blc_fw_inst = new blc_fw(1, 1, "blc_fw");
    lsc_hw* lsc_hw_inst = new lsc_hw(2, 1, "lsc_hw");
    lsc_fw* lsc_fw_inst = new lsc_fw(1, 1, "lsc_fw");
    ae_stat_hw* ae_stat_hw_inst = new ae_stat_hw(2, 0, "ae_stat_hw");
    ae_stat_fw* ae_stat_fw_inst = new ae_stat_fw(1, 1, "ae_stat_fw");
    awbgain_hw* awbgain_hw_inst = new awbgain_hw(2, 1, "awbgain_hw");
    awbgain_fw* awbgain_fw_inst = new awbgain_fw(1, 1, "awbgain_fw");
    demosaic_hw* demosaic_hw_inst = new demosaic_hw(2, 3, "demosaic_hw");
    demosaic_fw* demosaic_fw_inst = new demosaic_fw(1, 1, "demosaic_fw");
    cc_hw* cc_hw_inst = new cc_hw(4, 3, "cc_hw");
    cc_fw* cc_fw_inst = new cc_fw(1, 1, "cc_fw");
    hsv_lut_hw* hsv_lut_hw_inst = new hsv_lut_hw(4, 3, "hsv_lut_hw");
    hsv_lut_fw* hsv_lut_fw_inst = new hsv_lut_fw(1, 1, "hsv_lut_fw");
    prophoto2srgb_hw* prophoto2srgb_hw_inst = new prophoto2srgb_hw(4, 3, "prophoto2srgb_hw");
    prophoto2srgb_fw* prophoto2srgb_fw_inst = new prophoto2srgb_fw(1, 1, "prophoto2srgb_fw");
    gtm_stat_hw* gtm_stat_hw_inst = new gtm_stat_hw(4, 1, "gtm_stat_hw");
    gtm_stat_fw* gtm_stat_fw_inst = new gtm_stat_fw(1, 1, "gtm_stat_fw");
    gtm_hw* gtm_hw_inst = new gtm_hw(4, 3, "gtm_hw");
    gtm_fw* gtm_fw_inst = new gtm_fw(1, 1, "gtm_fw");
    Gamma_hw* gamma_hw_inst = new Gamma_hw(4, 3, "gamma_hw");
    Gamma_fw* gamma_fw_inst = new Gamma_fw(1, 1, "gamma_fw");
    rgb2yuv_hw* rgb2yuv_hw_inst = new rgb2yuv_hw(4, 3, "rgb2yuv_hw");
    rgb2yuv_fw* rgb2yuv_fw_inst = new rgb2yuv_fw(1, 1, "rgb2yuv_fw");
    yuv422_conv_hw* yuv422_conv_hw_inst = new yuv422_conv_hw(4, 3, "yuv422_conv_hw");
    yuv422_conv_fw* yuv422_conv_fw_inst = new yuv422_conv_fw(1, 1, "yuv422_conv_fw");
    yuv_encode_hw* yuv_encode_hw_inst = new yuv_encode_hw(3, 0, "yuv_encode_hw");

    manager->register_hw_module(raw_in);
    manager->register_hw_module(fe_fw_manager);
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

    manager->connect_port(raw_in, 0, fe_fw_manager, 0); //raw data
    manager->connect_port(fe_fw_manager, 0, raw_crop_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, raw_crop_hw_inst, 1);  //regs

    manager->connect_port(raw_crop_hw_inst, 0, blc_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, blc_hw_inst, 1);  //regs

    manager->connect_port(blc_hw_inst, 0, lsc_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, lsc_hw_inst, 1);  //regs

    manager->connect_port(lsc_hw_inst, 0, ae_stat_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, ae_stat_hw_inst, 1); //regs

    manager->connect_port(lsc_hw_inst, 0, awbgain_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, awbgain_hw_inst, 1); //regs

    manager->connect_port(awbgain_hw_inst, 0, demosaic_hw_inst, 0); //raw data
    manager->connect_port(fe_fw_manager, 1, demosaic_hw_inst, 1); //regs

    manager->connect_port(demosaic_hw_inst, 0, cc_hw_inst, 0); //r
    manager->connect_port(demosaic_hw_inst, 1, cc_hw_inst, 1); //g
    manager->connect_port(demosaic_hw_inst, 2, cc_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, cc_hw_inst, 3); //regs

    manager->connect_port(cc_hw_inst, 0, gtm_stat_hw_inst, 0); //r
    manager->connect_port(cc_hw_inst, 1, gtm_stat_hw_inst, 1); //g
    manager->connect_port(cc_hw_inst, 2, gtm_stat_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, gtm_stat_hw_inst, 3); //regs

    manager->connect_port(cc_hw_inst, 0, hsv_lut_hw_inst, 0); //r
    manager->connect_port(cc_hw_inst, 1, hsv_lut_hw_inst, 1); //g
    manager->connect_port(cc_hw_inst, 2, hsv_lut_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, hsv_lut_hw_inst, 3); //regs

    manager->connect_port(hsv_lut_hw_inst, 0, prophoto2srgb_hw_inst, 0); //r
    manager->connect_port(hsv_lut_hw_inst, 1, prophoto2srgb_hw_inst, 1); //g
    manager->connect_port(hsv_lut_hw_inst, 2, prophoto2srgb_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, prophoto2srgb_hw_inst, 3); //regs

    manager->connect_port(prophoto2srgb_hw_inst, 0, gtm_hw_inst, 0); //r
    manager->connect_port(prophoto2srgb_hw_inst, 1, gtm_hw_inst, 1); //g
    manager->connect_port(prophoto2srgb_hw_inst, 2, gtm_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, gtm_hw_inst, 3); //regs

    manager->connect_port(gtm_hw_inst, 0, gamma_hw_inst, 0); //r
    manager->connect_port(gtm_hw_inst, 1, gamma_hw_inst, 1); //g
    manager->connect_port(gtm_hw_inst, 2, gamma_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, gamma_hw_inst, 3); //regs

    manager->connect_port(gamma_hw_inst, 0, rgb2yuv_hw_inst, 0); //r
    manager->connect_port(gamma_hw_inst, 1, rgb2yuv_hw_inst, 1); //g
    manager->connect_port(gamma_hw_inst, 2, rgb2yuv_hw_inst, 2); //b
    manager->connect_port(fe_fw_manager, 1, rgb2yuv_hw_inst, 3); //regs

    manager->connect_port(rgb2yuv_hw_inst, 0, yuv422_conv_hw_inst, 0); //y
    manager->connect_port(rgb2yuv_hw_inst, 1, yuv422_conv_hw_inst, 1); //u
    manager->connect_port(rgb2yuv_hw_inst, 2, yuv422_conv_hw_inst, 2); //v
    manager->connect_port(fe_fw_manager, 1, yuv422_conv_hw_inst, 3); //regs

    manager->connect_port(yuv422_conv_hw_inst, 0, yuv_encode_hw_inst, 0); //y
    manager->connect_port(yuv422_conv_hw_inst, 1, yuv_encode_hw_inst, 1); //u
    manager->connect_port(yuv422_conv_hw_inst, 2, yuv_encode_hw_inst, 2); //v

    fe_fw_manager->regsiter_fw_modules(raw_crop_fw_inst);
    fe_fw_manager->regsiter_fw_modules(blc_fw_inst);
    fe_fw_manager->regsiter_fw_modules(lsc_fw_inst);
    fe_fw_manager->regsiter_fw_modules(ae_stat_fw_inst);
    fe_fw_manager->regsiter_fw_modules(awbgain_fw_inst);
    fe_fw_manager->regsiter_fw_modules(demosaic_fw_inst);
    fe_fw_manager->regsiter_fw_modules(cc_fw_inst);
    fe_fw_manager->regsiter_fw_modules(hsv_lut_fw_inst);
    fe_fw_manager->regsiter_fw_modules(prophoto2srgb_fw_inst);
    fe_fw_manager->regsiter_fw_modules(gtm_stat_fw_inst);
    fe_fw_manager->regsiter_fw_modules(gtm_fw_inst);
    fe_fw_manager->regsiter_fw_modules(gamma_fw_inst);
    fe_fw_manager->regsiter_fw_modules(rgb2yuv_fw_inst);
    fe_fw_manager->regsiter_fw_modules(yuv422_conv_fw_inst);

    fe_fw_manager->set_glb_ref(manager->global_ref_out);
    fe_fw_manager->set_cfg_file_name(&manager->cfg_file_name);
}