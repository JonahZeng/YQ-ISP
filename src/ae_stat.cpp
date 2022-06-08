#include "fe_firmware.h"

ae_stat::ae_stat(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name), start_x(32), start_y(32)
{
    ae_stat_reg = new ae_stat_reg_t;
    block_width = 0;
    block_height = 0;
    skip_step_x = 0;
    skip_step_y = 0;
    bypass = 0;
}

static void ae_stat_hw_core(uint16_t* tmp, uint32_t xsize, uint32_t ysize, bayer_type_t bayer_pattern, ae_stat_reg_t* ae_stat_reg, statistic_info_t* stat_out)
{
    memset(&stat_out->ae_stat, 0, sizeof(ae_stat_info_t));
    pixel_bayer_type pixel_all_type[4];
    if (bayer_pattern == RGGB)
    {
        pixel_all_type[0] = PIX_R;
        pixel_all_type[1] = PIX_GR;
        pixel_all_type[2] = PIX_GB;
        pixel_all_type[3] = PIX_B;
    }
    else if (bayer_pattern == GRBG)
    {
        pixel_all_type[0] = PIX_GR;
        pixel_all_type[1] = PIX_R;
        pixel_all_type[2] = PIX_B;
        pixel_all_type[3] = PIX_GB;
    }
    else if (bayer_pattern == GBRG)
    {
        pixel_all_type[0] = PIX_GB;
        pixel_all_type[1] = PIX_B;
        pixel_all_type[2] = PIX_R;
        pixel_all_type[3] = PIX_GR;
    }
    else if (bayer_pattern == BGGR)
    {
        pixel_all_type[0] = PIX_B;
        pixel_all_type[1] = PIX_GB;
        pixel_all_type[2] = PIX_GR;
        pixel_all_type[3] = PIX_R;
    }

    uint32_t block_id_x = 0;
    uint32_t block_id_y = 0;

    uint32_t pixel_idx;

    for (uint32_t row = 0; row < ysize; row++)
    {
        if (row >= ae_stat_reg->start_y[block_id_y] + ae_stat_reg->block_height)
        {
            block_id_y += 1;
        }
        block_id_x = 0;
        for (uint32_t col = 0; col < xsize; col++)
        {
            if (col >= ae_stat_reg->start_x[block_id_x] + ae_stat_reg->block_width)
            {
                block_id_x += 1;
            }
            if (col >= ae_stat_reg->start_x[block_id_x] && col < ae_stat_reg->start_x[block_id_x] + ae_stat_reg->block_width 
                && row >= ae_stat_reg->start_y[block_id_y] && row < ae_stat_reg->start_y[block_id_y] + ae_stat_reg->block_height
                && (col % ae_stat_reg->skip_step_x) < 2 && (row % ae_stat_reg->skip_step_y) < 2)
            {
                pixel_idx = (row % 2) * 2 + (col % 2);
                pixel_bayer_type type_p = pixel_all_type[pixel_idx];
                if (type_p == PIX_R)
                {
                    stat_out->ae_stat.r_cnt[block_id_y * 32 + block_id_x] += 1;
                    stat_out->ae_stat.r_sum[block_id_y * 32 + block_id_x] += tmp[row*xsize + col];
                }
                else if (type_p == PIX_GR || type_p == PIX_GB)
                {
                    stat_out->ae_stat.g_cnt[block_id_y * 32 + block_id_x] += 1;
                    stat_out->ae_stat.g_sum[block_id_y * 32 + block_id_x] += tmp[row*xsize + col];
                }
                else if (type_p == PIX_B)
                {
                    stat_out->ae_stat.b_cnt[block_id_y * 32 + block_id_x] += 1;
                    stat_out->ae_stat.b_sum[block_id_y * 32 + block_id_x] += tmp[row*xsize + col];
                }
            }
        }
        
    }
}

void ae_stat::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    if (in.size() > 1)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
        memcpy(ae_stat_reg, &fe_reg->ae_stat_reg, sizeof(ae_stat_reg_t));
    }
    ae_stat_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        ae_stat_reg->block_width = block_width;
        ae_stat_reg->block_height = block_height;
        ae_stat_reg->skip_step_x = skip_step_x;
        ae_stat_reg->skip_step_y = skip_step_y;
        for (int32_t i = 0; i < 32; i++)
        {
            ae_stat_reg->start_x[i] = start_x[i];
            ae_stat_reg->start_y[i] = start_y[i];
        }
    }

    checkparameters(ae_stat_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    std::unique_ptr<uint16_t[]> tmp_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp = tmp_ptr.get();

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 14);
    }

    if (ae_stat_reg->bypass == 0)
    {
        ae_stat_hw_core(tmp, xsize, ysize, bayer_pattern, ae_stat_reg, stat_out);
    }

    std::unique_ptr<uint32_t[]> r_mean_ptr(new uint32_t[1024]);
    uint32_t* r_mean = r_mean_ptr.get();
    std::unique_ptr<uint32_t[]> g_mean_ptr(new uint32_t[1024]);
    uint32_t* g_mean = g_mean_ptr.get();
    std::unique_ptr<uint32_t[]> b_mean_ptr(new uint32_t[1024]);
    uint32_t* b_mean = b_mean_ptr.get();
    for (int32_t i = 0; i < 1024; i++)
    {
        r_mean[i] = stat_out->ae_stat.r_sum[i] / stat_out->ae_stat.r_cnt[i];
        g_mean[i] = stat_out->ae_stat.g_sum[i] / stat_out->ae_stat.g_cnt[i];
        b_mean[i] = stat_out->ae_stat.b_sum[i] / stat_out->ae_stat.b_cnt[i];
    }

    log_array("ae_stat r:\n", "%5d, ", r_mean, 1024, 32);
    log_array("ae_stat g:\n", "%5d, ", g_mean, 1024, 32);
    log_array("ae_stat b:\n", "%5d, ", b_mean, 1024, 32);

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void ae_stat::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                       UINT_32,          &this->bypass                },
        {"block_width",                  UINT_32,          &this->block_width           },
        {"block_height",                 UINT_32,          &this->block_height          },
        {"skip_step_x",                  UINT_32,          &this->skip_step_x           },
        {"skip_step_y",                  UINT_32,          &this->skip_step_y           },
        {"start_x",                      VECT_UINT32,      &this->start_x,            32},
        {"start_y",                      VECT_UINT32,      &this->start_y,            32}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

ae_stat::~ae_stat()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (ae_stat_reg != nullptr)
    {
        delete ae_stat_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void ae_stat::checkparameters(ae_stat_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->block_width = common_check_bits(reg->block_width, 9, "block_width");
    reg->block_height = common_check_bits(reg->block_height, 9, "block_height");
    reg->skip_step_x = common_check_bits(reg->skip_step_x, 4, "skip_step_x");
    reg->skip_step_y = common_check_bits(reg->skip_step_y, 4, "skip_step_y");
    for (int32_t i = 0; i < 32; i++)
    {
        reg->start_x[i] = common_check_bits(reg->start_x[i], 13, "start_x");
        reg->start_y[i] = common_check_bits(reg->start_y[i], 13, "start_y");
    }

    log_info("================= %s reg=================\n", name);
    log_info("bypass %d\n", reg->bypass);
    log_info("block_width %d\n", reg->block_width);
    log_info("block_height %d\n", reg->block_height);
    log_info("skip_step_x %d\n", reg->skip_step_x);
    log_info("skip_step_y %d\n", reg->skip_step_y);
    log_array("start x:\n", "%5d, ", reg->start_x, 32, 8);
    log_array("start y:\n", "%5d, ", reg->start_y, 32, 8);
    log_info("================= %s reg=================\n", name);
}