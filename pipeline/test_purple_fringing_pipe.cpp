#include "pipeline_manager.h"
#include "yuv_decode_hw.h"
#include "yuv_encode_hw.h"
#include "yuv422_conv_hw.h"
#include "yuv2rgb_8bit_hw.h"
#include "cac_rgb_hw.h"
#include "rgb2yuv_hw.h"


void test_purple_fringing_pipe(pipeline_manager* manager)
{
    yuv_decode_hw* yuv_decode_hw_inst = new yuv_decode_hw(0, 3, "yuv_decode_hw");
    yuv2rgb_8b_hw* yuv2rgb_hw_inst = new yuv2rgb_8b_hw(3, 3, "yuv2rgb_hw");
    cac_rgb_hw* cac_rgb_hw_inst = new cac_rgb_hw(3, 3, "cac_rgb_hw");
    rgb2yuv_hw* rgb2yuv_hw_inst = new rgb2yuv_hw(3, 3, "rgb2yuv_hw");
    yuv422_conv_hw* yuv422_conv_hw_inst = new yuv422_conv_hw(3, 3, "yuv422_conv_hw");
    yuv_encode_hw* yuv_encode_hw_inst = new yuv_encode_hw(3, 0, "yuv_encode_hw");

    manager->register_hw_module(yuv_decode_hw_inst);
    manager->register_hw_module(yuv2rgb_hw_inst);
    manager->register_hw_module(cac_rgb_hw_inst);
    manager->register_hw_module(rgb2yuv_hw_inst);
    manager->register_hw_module(yuv422_conv_hw_inst);
    manager->register_hw_module(yuv_encode_hw_inst);

    manager->connect_port(yuv_decode_hw_inst, 0, yuv2rgb_hw_inst, 0); //y
    manager->connect_port(yuv_decode_hw_inst, 1, yuv2rgb_hw_inst, 1); //u
    manager->connect_port(yuv_decode_hw_inst, 2, yuv2rgb_hw_inst, 2); //v

    manager->connect_port(yuv2rgb_hw_inst, 0, cac_rgb_hw_inst, 0); //r
    manager->connect_port(yuv2rgb_hw_inst, 1, cac_rgb_hw_inst, 1); //g
    manager->connect_port(yuv2rgb_hw_inst, 2, cac_rgb_hw_inst, 2); //b

    manager->connect_port(cac_rgb_hw_inst, 0, rgb2yuv_hw_inst, 0); //r
    manager->connect_port(cac_rgb_hw_inst, 1, rgb2yuv_hw_inst, 1); //g
    manager->connect_port(cac_rgb_hw_inst, 2, rgb2yuv_hw_inst, 2); //b

    manager->connect_port(rgb2yuv_hw_inst, 0, yuv422_conv_hw_inst, 0); //y
    manager->connect_port(rgb2yuv_hw_inst, 1, yuv422_conv_hw_inst, 1); //u
    manager->connect_port(rgb2yuv_hw_inst, 2, yuv422_conv_hw_inst, 2); //v

    manager->connect_port(yuv422_conv_hw_inst, 0, yuv_encode_hw_inst, 0); //y
    manager->connect_port(yuv422_conv_hw_inst, 1, yuv_encode_hw_inst, 1); //u
    manager->connect_port(yuv422_conv_hw_inst, 2, yuv_encode_hw_inst, 2); //v
}