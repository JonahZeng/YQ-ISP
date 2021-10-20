#include "fe_firmware.h"
#include <sstream>

#define CLIP3(x, min, max) (x>max)?max:((x<min)?min:x)

gtm_stat::gtm_stat(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    rgb2y.resize(3);
    w_ratio = 2;
    h_ratio = 2;
}

static void gtm_stat_hw_core(gtm_stat_info_t* stat_gtm, uint16_t* r, uint16_t* g, uint16_t* b, uint32_t xsize, uint32_t ysize, const gtm_stat_reg_t* gtm_stat_reg)
{
    const uint32_t* rgb2y = gtm_stat_reg->rgb2y;
    uint16_t tmp_y = 0;
    uint16_t idx;
    for (uint32_t row = 0; row < ysize; row += gtm_stat_reg->h_ratio)
    {
        for (uint32_t col = 0; col < xsize; col += gtm_stat_reg->w_ratio)
        {
            tmp_y = (r[row*xsize + col] * rgb2y[0] + g[row*xsize + col] * rgb2y[1] + b[row*xsize + col] * rgb2y[2] + 512) >> 10;
            tmp_y = CLIP3(tmp_y, 0, 16383);
            idx = tmp_y / 64;
            idx = CLIP3(idx, 0, 255);

            stat_gtm->luma_hist[idx] += 1;
            stat_gtm->total_pixs += 1;
        }
    }
}

void gtm_stat::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
    gtm_stat_reg_t* gtm_stat_reg = &fe_reg->gtm_stat_reg;

    gtm_stat_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        gtm_stat_reg->rgb2y[0] = rgb2y[0];
        gtm_stat_reg->rgb2y[1] = rgb2y[1];
        gtm_stat_reg->rgb2y[2] = rgb2y[2];
        gtm_stat_reg->w_ratio = w_ratio;
        gtm_stat_reg->h_ratio = h_ratio;
    }

    checkparameters(gtm_stat_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    uint16_t* in_r = input0->data_ptr;
    uint16_t* in_g = input1->data_ptr;
    uint16_t* in_b = input2->data_ptr;

    uint32_t output_size = sizeof(gtm_stat_info_t)/sizeof(uint16_t);

    data_buffer* output0 = new data_buffer(output_size, 1, DATA_TYPE_UNSUPPORT, BAYER_UNSUPPORT, "gtm_stat_out0");
    out[0] = output0;

    gtm_stat_info_t* stat_gtm = (gtm_stat_info_t*)(output0->data_ptr);
    memset(stat_gtm, 0, sizeof(gtm_stat_info_t));

    if (gtm_stat_reg->bypass == 0)
    {
        stat_gtm->stat_en = 1;
        gtm_stat_hw_core(stat_gtm, in_r, in_g, in_b, xsize, ysize, gtm_stat_reg);
    }
    else 
    {
        stat_gtm->stat_en = 0;
    }
#ifdef _MSC_VER
    memcpy_s(&stat_out->gtm_stat, sizeof(gtm_stat_info_t), stat_gtm, sizeof(gtm_stat_info_t));
#else
    memcpy(&stat_out->gtm_stat, stat_gtm, sizeof(gtm_stat_info_t));
#endif

    std::stringstream ostr;
    for (int32_t i = 0; i < 256; i++)
    {
        ostr << stat_gtm->luma_hist[i] << ", ";
        if (i % 32 == 31)
        {
            ostr << std::endl;
        }
    }
    spdlog::info("luma hist[256]\n{}", ostr.str());

    hw_base::hw_run(stat_out, frame_cnt);
    spdlog::info("{0} run end", __FUNCTION__);
}


void gtm_stat::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,         &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}

gtm_stat::~gtm_stat()
{
    spdlog::info("{0} module deinit start", __FUNCTION__);
    spdlog::info("{0} module deinit end", __FUNCTION__);
}

void gtm_stat::checkparameters(gtm_stat_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    for (int32_t i = 0; i < 3; i++)
    {
        reg->rgb2y[i] = common_check_bits(reg->rgb2y[i], 10, "rgb2y");
    }
    reg->w_ratio = common_check_bits(reg->w_ratio, 4, "w_ratio");
    reg->h_ratio = common_check_bits(reg->h_ratio, 4, "h_ratio");
    spdlog::info("================= gtm stat reg=================");
    spdlog::info("bypass {}", reg->bypass);
    spdlog::info("rgb2y {} {} {}", reg->rgb2y[0], reg->rgb2y[1], reg->rgb2y[2]);
    spdlog::info("w_ratio {} h_ratio {}", reg->w_ratio, reg->h_ratio);
    spdlog::info("================= gtm stat reg=================");
}