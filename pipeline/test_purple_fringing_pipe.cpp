#include "pipeline_manager.h"
#include "yuv_decode.h"
#include "yuv_encode.h"
#include "yuv422_conv.h"
#include "yuv2rgb_8bit.h"
#include "cac_rgb.h"
#include "rgb2yuv.h"


void test_purple_fringing_pipe(pipeline_manager* manager)
{
    yuv_decode* yuv_decode_hw = new yuv_decode(0, 3, "yuv_decode_hw");
    yuv2rgb_8b* yuv2rgb_hw = new yuv2rgb_8b(3, 3, "yuv2rgb_hw");
    cac_rgb* cac_rgb_hw = new cac_rgb(3, 3, "cac_rgb_hw");
    rgb2yuv* rgb2yuv_hw = new rgb2yuv(3, 3, "rgb2yuv_hw");
    yuv422_conv* yuv422_conv_hw = new yuv422_conv(3, 3, "yuv422_conv_hw");
    yuv_encode* yuv_encode_hw = new yuv_encode(3, 0, "yuv_encode_hw");

    manager->register_hw_module(yuv_decode_hw);
    manager->register_hw_module(yuv2rgb_hw);
    manager->register_hw_module(cac_rgb_hw);
    manager->register_hw_module(rgb2yuv_hw);
    manager->register_hw_module(yuv422_conv_hw);
    manager->register_hw_module(yuv_encode_hw);

    manager->connect_port(yuv_decode_hw, 0, yuv2rgb_hw, 0); //y
    manager->connect_port(yuv_decode_hw, 1, yuv2rgb_hw, 1); //u
    manager->connect_port(yuv_decode_hw, 2, yuv2rgb_hw, 2); //v

    manager->connect_port(yuv2rgb_hw, 0, cac_rgb_hw, 0); //r
    manager->connect_port(yuv2rgb_hw, 1, cac_rgb_hw, 1); //g
    manager->connect_port(yuv2rgb_hw, 2, cac_rgb_hw, 2); //b

    manager->connect_port(cac_rgb_hw, 0, rgb2yuv_hw, 0); //r
    manager->connect_port(cac_rgb_hw, 1, rgb2yuv_hw, 1); //g
    manager->connect_port(cac_rgb_hw, 2, rgb2yuv_hw, 2); //b

    manager->connect_port(rgb2yuv_hw, 0, yuv422_conv_hw, 0); //y
    manager->connect_port(rgb2yuv_hw, 1, yuv422_conv_hw, 1); //u
    manager->connect_port(rgb2yuv_hw, 2, yuv422_conv_hw, 2); //v

    manager->connect_port(yuv422_conv_hw, 0, yuv_encode_hw, 0); //y
    manager->connect_port(yuv422_conv_hw, 1, yuv_encode_hw, 1); //u
    manager->connect_port(yuv422_conv_hw, 2, yuv_encode_hw, 2); //v
}