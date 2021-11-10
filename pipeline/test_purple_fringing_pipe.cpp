#include "pipeline_manager.h"
#include "yuv_decode.h"
#include "yuv_encode.h"
#include "yuv422_conv.h"


void test_purple_fringing_pipe(pipeline_manager* manager)
{
    yuv_decode* yuv_decode_hw = new yuv_decode(0, 3, "yuv_decode_hw");
    yuv422_conv* yuv422_conv_hw = new yuv422_conv(3, 3, "yuv422_conv_hw");
    yuv_encode* yuv_encode_hw = new yuv_encode(3, 0, "yuv_encode_hw");

    manager->register_module(yuv_decode_hw);
    manager->register_module(yuv422_conv_hw);
    manager->register_module(yuv_encode_hw);

    yuv_decode_hw->connect_port(0, yuv422_conv_hw, 0); //y
    yuv_decode_hw->connect_port(1, yuv422_conv_hw, 1); //u
    yuv_decode_hw->connect_port(2, yuv422_conv_hw, 2); //v

    yuv422_conv_hw->connect_port(0, yuv_encode_hw, 0); //y
    yuv422_conv_hw->connect_port(1, yuv_encode_hw, 1); //u
    yuv422_conv_hw->connect_port(2, yuv_encode_hw, 2); //v
}