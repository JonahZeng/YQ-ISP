#include "demosaic_hw.h"
#include "pipe_register.h"
#include <memory>

extern void demosaic_hw_core_eig(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                             uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg);
extern void demosaic_hw_core_vng(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                             uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg);

demosaic_hw::demosaic_hw(uint32_t inpins, uint32_t outpins, const char *inst_name) : hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    demosaic_reg = new demosaic_reg_t;
}

static pixel_bayer_type get_pixel_bayer_type(uint32_t y, uint32_t x, bayer_type_t by)
{
    uint32_t pos = ((y % 2) << 1) + (x % 2);

    pixel_bayer_type type[4];
    if (by == RGGB)
    {
        type[0] = PIX_R;
        type[1] = PIX_GR;
        type[2] = PIX_GB;
        type[3] = PIX_B;
    }
    else if (by == GRBG)
    {
        type[0] = PIX_GR;
        type[1] = PIX_R;
        type[2] = PIX_B;
        type[3] = PIX_GB;
    }
    else if (by == GBRG)
    {
        type[0] = PIX_GB;
        type[1] = PIX_B;
        type[2] = PIX_R;
        type[3] = PIX_GR;
    }
    else if (by == BGGR)
    {
        type[0] = PIX_B;
        type[1] = PIX_GB;
        type[2] = PIX_GR;
        type[3] = PIX_R;
    }

    return type[pos];
}

static void demosaic_hw_core_bilinear(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                                      uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg)
{
    pixel_bayer_type pix_type;
    uint32_t x_, y_;
    uint16_t r_0, r_1, r_2, r_3;
    uint16_t g_0, g_1, g_2, g_3;
    uint16_t b_0, b_1, b_2, b_3;
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;
            if (pix_type == PIX_R)
            {
                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                g_out[row * xsize + col] = (g_0 + g_1 + g_2 + g_3 + 2) / 4; // 16383 *4 = 65532

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                b_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                b_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (b_0 + b_1 + b_2 + b_3 + 2) / 4;
            }
            else if (pix_type == PIX_GR)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (r_0 + r_1 + 1) / 2;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                b_out[row * xsize + col] = (b_0 + b_1 + 1) / 2;
            }
            else if (pix_type == PIX_GB)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (b_0 + b_1 + 1) / 2;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                r_out[row * xsize + col] = (r_0 + r_1 + 1) / 2;
            }
            else if (pix_type == PIX_B)
            {
                b_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                g_out[row * xsize + col] = (g_0 + g_1 + g_2 + g_3 + 2) / 4; // 16383 *4 = 65532

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                r_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                r_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (r_0 + r_1 + r_2 + r_3 + 2) / 4;
            }
        }
    }
}

static void demosaic_hw_core_imp_linear(uint16_t *indata, uint16_t *r_out, uint16_t *g_out, uint16_t *b_out,
                                        uint32_t xsize, uint32_t ysize, uint32_t EXT_X, uint32_t EXT_Y, bayer_type_t by, const demosaic_reg_t *demosiac_reg)
{
    pixel_bayer_type pix_type;
    uint32_t x_, y_;
    uint32_t r_0, r_1, r_2, r_3, r_c;
    uint32_t g_0, g_1, g_2, g_3, g_4, g_5, g_6, g_7, g_c;
    uint32_t b_0, b_1, b_2, b_3, b_c;
    for (uint32_t row = 0; row < ysize; row++)
    {
        for (uint32_t col = 0; col < xsize; col++)
        {
            pix_type = get_pixel_bayer_type(row, col, by);
            y_ = row + EXT_Y / 2;
            x_ = col + EXT_X / 2;
            if (pix_type == PIX_R)
            {
                r_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];

                r_c = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                r_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                r_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_out[row * xsize + col] = (2 * (g_0 + g_1 + g_2 + g_3) + 4 * r_c - r_0 - r_1 - r_2 - r_3 + 4) >> 3;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                b_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                b_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                b_out[row * xsize + col] = (4 * (b_0 + b_1 + b_2 + b_3) + 12 * r_c - 3 * (r_0 + r_1 + r_2 + r_3) + 8) >> 4;
            }
            else if (pix_type == PIX_GR)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                r_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];

                g_c = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                g_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                g_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                g_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_4 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                g_5 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                g_6 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                g_7 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (8 * (r_0 + r_1) + 10 * g_c + g_0 + g_1 - 2 * (g_2 + g_3 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;

                b_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                b_out[row * xsize + col] = (8 * (b_0 + b_1) + 10 * g_c + g_2 + g_3 - 2 * (g_0 + g_1 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;
            }
            else if (pix_type == PIX_GB)
            {
                g_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                b_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];

                g_c = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                g_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                g_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                g_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];
                g_4 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                g_5 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                g_6 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                g_7 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];

                b_out[row * xsize + col] = (8 * (b_0 + b_1 + 1) + 10 * g_c + g_0 + g_1 - 2 * (g_2 + g_3 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                r_1 = indata[(y_ + 1) * (xsize + EXT_X) + x_];
                r_out[row * xsize + col] = (8 * (r_0 + r_1) + 10 * g_c + g_2 + g_3 - 2 * (g_0 + g_1 + g_4 + g_5 + g_6 + g_7) + 8) >> 4;
            }
            else if (pix_type == PIX_B)
            {
                b_out[row * xsize + col] = indata[y_ * (xsize + EXT_X) + x_];
                g_0 = indata[y_ * (xsize + EXT_X) + x_ - 1];
                g_1 = indata[y_ * (xsize + EXT_X) + x_ + 1];
                g_2 = indata[(y_ - 1) * (xsize + EXT_X) + x_];
                g_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_];

                b_c = indata[y_ * (xsize + EXT_X) + x_];
                b_0 = indata[(y_ - 2) * (xsize + EXT_X) + x_];
                b_1 = indata[(y_ + 2) * (xsize + EXT_X) + x_];
                b_2 = indata[y_ * (xsize + EXT_X) + x_ - 2];
                b_3 = indata[y_ * (xsize + EXT_X) + x_ + 2];

                g_out[row * xsize + col] = (2 * (g_0 + g_1 + g_2 + g_3) + 4 * b_c - (b_0 + b_1 + b_2 + b_3) + 4) >> 3;

                r_0 = indata[(y_ - 1) * (xsize + EXT_X) + x_ - 1];
                r_1 = indata[(y_ - 1) * (xsize + EXT_X) + x_ + 1];
                r_2 = indata[(y_ + 1) * (xsize + EXT_X) + x_ - 1];
                r_3 = indata[(y_ + 1) * (xsize + EXT_X) + x_ + 1];
                r_out[row * xsize + col] = (4 * (r_0 + r_1 + r_2 + r_3) + 12 * b_c - 3 * (b_0 + b_1 + b_2 + b_3) + 8) >> 4;
            }

            r_out[row * xsize + col] = (r_out[row * xsize + col] > 16383) ? 16383 : r_out[row * xsize + col];
            g_out[row * xsize + col] = (g_out[row * xsize + col] > 16383) ? 16383 : g_out[row * xsize + col];
            b_out[row * xsize + col] = (b_out[row * xsize + col] > 16383) ? 16383 : b_out[row * xsize + col];
        }
    }
}



void demosaic_hw::hw_run(statistic_info_t *stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer *input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    if (in.size() > 1)
    {
        fe_module_reg_t *fe_reg = (fe_module_reg_t *)(in[1]->data_ptr);
        memcpy(demosaic_reg, &fe_reg->demosaic_reg, sizeof(demosaic_reg_t));
    }

    demosaic_reg->bypass = bypass;
    if (xmlConfigValid)
    {
    }

    checkparameters(demosaic_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer *output_r = new data_buffer(xsize, ysize, DATA_TYPE_R, BAYER_UNSUPPORT, "demosaic_out0");
    uint16_t *r_ptr = output_r->data_ptr;
    out[0] = output_r;

    data_buffer *output_g = new data_buffer(xsize, ysize, DATA_TYPE_G, BAYER_UNSUPPORT, "demosaic_out1");
    uint16_t *g_ptr = output_g->data_ptr;
    out[1] = output_g;

    data_buffer *output_b = new data_buffer(xsize, ysize, DATA_TYPE_B, BAYER_UNSUPPORT, "demosaic_out2");
    uint16_t *b_ptr = output_b->data_ptr;
    out[2] = output_b;

    const uint32_t EXT_W = 8; // 2;
    const uint32_t EXT_H = 8; // 2;

    std::unique_ptr<uint16_t[]> tmp_data(new uint16_t[(xsize + EXT_W) * (ysize + EXT_H)]);
    uint16_t *tmp = tmp_data.get(); // new uint16_t[(xsize + EXT_W)*(ysize + EXT_H)]; //ext boarder

    for (uint32_t y = EXT_H / 2; y < ysize + EXT_H / 2; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = input_raw->data_ptr[(y - EXT_H / 2) * xsize + x - EXT_W / 2] >> (16 - 14);
        }
    }
    // top
    for (uint32_t y = 0; y < EXT_H / 2; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[(EXT_H - y) * (xsize + EXT_W) + x];
        }
    }
    // bottom
    for (uint32_t y = ysize + EXT_H / 2; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = EXT_W / 2; x < xsize + EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[(2 * ysize + EXT_H - 2 - y) * (xsize + EXT_W) + x];
        }
    }
    // left
    for (uint32_t y = 0; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = 0; x < EXT_W / 2; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[y * (xsize + EXT_W) + (EXT_W - x)];
        }
    }
    // right
    for (uint32_t y = 0; y < ysize + EXT_H; y++)
    {
        for (uint32_t x = xsize + EXT_W / 2; x < xsize + EXT_W; x++)
        {
            tmp[y * (xsize + EXT_W) + x] = tmp[y * (xsize + EXT_W) + (2 * xsize + EXT_W - 2 - x)];
        }
    }

    if (demosaic_reg->bypass == 0)
    {
        demosaic_hw_core_eig(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
    }
    else if(demosaic_reg->bypass == 1)
    {
        demosaic_hw_core_vng(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
    }
    else
    {
        demosaic_hw_core_bilinear(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
        //demosaic_hw_core_imp_linear(tmp, r_ptr, g_ptr, b_ptr, xsize, ysize, EXT_W, EXT_H, bayer_pattern, demosaic_reg);
    }

    for (uint32_t sz = 0; sz < xsize * ysize; sz++)
    {
        r_ptr[sz] = r_ptr[sz] << (16 - 14);
        g_ptr[sz] = g_ptr[sz] << (16 - 14);
        b_ptr[sz] = b_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void demosaic_hw::hw_init()
{
    log_info("%s init run start\n", name);

    cfgEntry_t config[] = {
        {"bypass", UINT_32, &this->bypass}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

demosaic_hw::~demosaic_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (demosaic_reg != nullptr)
    {
        delete demosaic_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void demosaic_hw::checkparameters(demosaic_reg_t *reg)
{
    reg->bypass = common_check_bits(reg->bypass, 2, "bypass");
}
