#include "pipe_register.h"

#define FLIP(x, max_v) ((x) < 0 ? (-(x)) : ((x) > (max_v) ? 2 * (max_v) - (x) : (x)))
#define CLIP3(x, min_v, max_v) ((x) < (min_v) ? (min_v) : ((x) > (max_v) ? (max_v) : (x)))

#define REPEAT4(x, max_v) ((x) < 0 ? (4 + (x)) : ((x) > (max_v) ? ((x)-4) : (x)))

rgbir_remosaic_hw::rgbir_remosaic_hw(uint32_t inpins, uint32_t outpins, const char* inst_name)
    :hw_base(inpins, outpins, inst_name), bypass(0), rgbir_remosaic_reg(new rgbir_remosaic_reg_t)
{

}

static pixel_bayer_type get_pixel_bayer_type_ir(uint32_t y, uint32_t x, bayer_type_t by)
{
    uint32_t pos = ((y % 4) << 2) + (x % 4);

    pixel_bayer_type type[16];
    if (by == RGGIR)
    {
        type[0] = PIX_R; type[1] = PIX_GR; type[2] = PIX_B; type[3] = PIX_GB;
        type[4] = PIX_GB; type[5] = PIX_Y; type[6] = PIX_GR; type[7] = PIX_Y;
        type[8] = PIX_B; type[9] = PIX_GB; type[10] = PIX_R; type[11] = PIX_GR;
        type[12] = PIX_GR; type[13] = PIX_Y; type[14] = PIX_GB; type[15] = PIX_Y;
    }
    else if (by == BGGIR)
    {
        type[0] = PIX_B; type[1] = PIX_GB; type[2] = PIX_R; type[3] = PIX_GR;
        type[4] = PIX_GR; type[5] = PIX_Y; type[6] = PIX_GB; type[7] = PIX_Y;
        type[8] = PIX_R; type[9] = PIX_GR; type[10] = PIX_B; type[11] = PIX_GB;
        type[12] = PIX_GB; type[13] = PIX_Y; type[14] = PIX_GR; type[15] = PIX_Y;
    }
    else if (by == GRIRG)
    {
        type[0] = PIX_GR; type[1] = PIX_R; type[2] = PIX_GB; type[3] = PIX_B;
        type[4] = PIX_Y; type[5] = PIX_GB; type[6] = PIX_Y; type[7] = PIX_GR;
        type[8] = PIX_GB; type[9] = PIX_B; type[10] = PIX_GR; type[11] = PIX_R;
        type[12] = PIX_Y; type[13] = PIX_GR; type[14] = PIX_Y; type[15] = PIX_GB;
    }
    else if(by == GBIRG)
    {
        type[0] = PIX_GB; type[1] = PIX_B; type[2] = PIX_GR; type[3] = PIX_R;
        type[4] = PIX_Y; type[5] = PIX_GR; type[6] = PIX_Y; type[7] = PIX_GB;
        type[8] = PIX_GR; type[9] = PIX_R; type[10] = PIX_GB; type[11] = PIX_B;
        type[12] = PIX_Y; type[13] = PIX_GB; type[14] = PIX_Y; type[15] = PIX_GR;
    }


    return type[pos];
}

static void rgbir_remosaic_hw_core(uint16_t* src, uint16_t* out0_ptr, uint16_t* out1_ptr, uint32_t xsize, uint32_t ysize,
    bayer_type_t bayer_pattern, const rgbir_remosaic_reg_t* rgbir_remosaic_reg)
{

}

static void rgbir_remosaic_hw_core_blinear(uint16_t* src, uint16_t* out0_ptr, uint16_t* out1_ptr, uint32_t xsize, uint32_t ysize,
    bayer_type_t bayer_pattern, const rgbir_remosaic_reg_t* rgbir_remosaic_reg)
{
    //assume BGGIR RGGIR GRIRG GBIRG
    const int32_t MAX_ROW =  ysize - 1;
    const int32_t MAX_COL =  xsize - 1;
    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if(pix_type == PIX_R || pix_type == PIX_B)
            {
                uint16_t Y_lt = src[FLIP(row - 1, MAX_ROW) * xsize + FLIP(col - 1, MAX_COL)];
                uint16_t Y_rt = src[FLIP(row - 1, MAX_ROW) * xsize + FLIP(col + 1, MAX_COL)];
                uint16_t Y_lb = src[FLIP(row + 1, MAX_ROW) * xsize + FLIP(col - 1, MAX_COL)];
                uint16_t Y_rb = src[FLIP(row + 1, MAX_ROW) * xsize + FLIP(col + 1, MAX_COL)];
                uint16_t mean_val = (Y_lt + Y_rt + Y_lb + Y_rb + 2) / 4;
                out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
            }
            else if(pix_type == PIX_GR || pix_type == PIX_GB)
            {
                if(row % 2 == 0)
                {
                    uint16_t Y_t = src[FLIP(row - 1, MAX_ROW) * xsize + col];
                    uint16_t Y_b = src[FLIP(row + 1, MAX_ROW) * xsize + col];
                    uint16_t mean_val = (Y_t + Y_b + 1) / 2;
                    out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
                else{
                    uint16_t Y_l = src[row * xsize + FLIP(col - 1, MAX_COL)];
                    uint16_t Y_r = src[row * xsize + FLIP(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_l + Y_r + 1) / 2;
                    out1_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
            }
            else{
                out1_ptr[row*xsize + col] = src[row * xsize + col];
            }
        }
    }

    for(int32_t row=0; row<(int32_t)ysize; row++)
    {
        for(int32_t col=0; col<(int32_t)xsize; col++)
        {
            //assume BGGIR RGGIR GRIRG GBIRG
            pixel_bayer_type pix_type = get_pixel_bayer_type_ir((uint32_t)row, (uint32_t)col, bayer_pattern);
            if((bayer_pattern == RGGIR || bayer_pattern == BGGIR) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4)
                {
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
                else{
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
            }
            else if((bayer_pattern == GRIRG || bayer_pattern == GBIRG) && pix_type == PIX_Y) //replace ir with B/R
            {
                if (row % 4 == col % 4 + 1)
                {
                    uint16_t Y_lt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t Y_rb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t mean_val = (Y_lt + Y_rb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
                else{
                    uint16_t Y_rt = src[REPEAT4(row - 1, MAX_ROW) * xsize + REPEAT4(col + 1, MAX_COL)];
                    uint16_t Y_lb = src[REPEAT4(row + 1, MAX_ROW) * xsize + REPEAT4(col - 1, MAX_COL)];
                    uint16_t mean_val = (Y_rt + Y_lb + 1) / 2;
                    out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
                }
            }
            else if((bayer_pattern == RGGIR || bayer_pattern == GRIRG) && pix_type == PIX_B) //replace B with R
            {
                uint16_t R_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t R_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t R_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t R_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                uint16_t mean_val = (R_l + R_r + R_t + R_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
            }
            else if((bayer_pattern == BGGIR || bayer_pattern == GBIRG) && pix_type == PIX_R) //replace R with B
            {
                uint16_t B_l = src[row * xsize + FLIP(col - 2, MAX_COL)];
                uint16_t B_r = src[row * xsize + FLIP(col + 2, MAX_COL)];
                uint16_t B_t = src[FLIP(row - 2, MAX_ROW) * xsize + col];
                uint16_t B_b = src[FLIP(row + 2, MAX_ROW) * xsize + col];
                uint16_t mean_val = (B_l + B_r + B_t + B_b + 2) / 4;
                out0_ptr[row*xsize + col] = CLIP3(mean_val, 0, 4095);
            }
        }
    }
}

void rgbir_remosaic_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    // RGGIR = 4,
    // GRIRG = 5,
    // BGGIR = 6,
    // GBIRG = 7

    if(bayer_pattern != BGGIR && bayer_pattern != RGGIR && bayer_pattern != GRIRG && bayer_pattern != GBIRG)
    {
        log_error("rgbir_remosaic_hw not support %d pattern\n", bayer_pattern);
        exit(EXIT_FAILURE);
    }

    if (in.size() > 1)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
        memcpy(rgbir_remosaic_reg, &fe_reg->rgbir_remosaic_reg, sizeof(rgbir_remosaic_reg_t));
    }

    rgbir_remosaic_reg->bypass = bypass;
    if (xmlConfigValid)
    {

    }
    else{

    }

    checkparameters(rgbir_remosaic_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, "rgbir_remosaic_out0");
    uint16_t* out0_ptr = output0->data_ptr;
    out[0] = output0;
    if(input_raw->bayer_pattern == BGGIR)
    {
        output0->bayer_pattern = BGGR;
    }
    else if(input_raw->bayer_pattern == RGGIR)
    {
        output0->bayer_pattern = RGGB;
    }
    else if(input_raw->bayer_pattern == GRIRG)
    {
        output0->bayer_pattern = GRBG;
    }
    else if(input_raw->bayer_pattern == GBIRG)
    {
        output0->bayer_pattern = GBRG;
    }
    data_buffer* output1 = new data_buffer(xsize, ysize, DATA_TYPE_Y, BAYER_UNSUPPORT, "rgbir_remosaic_out1");
    uint16_t* out1_ptr = output1->data_ptr;
    out[1] = output1;

    std::unique_ptr<uint16_t[]> tmp_ptr(new uint16_t[xsize * ysize]);
    uint16_t* tmp = tmp_ptr.get();

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 12);
        out0_ptr[sz] = tmp[sz];
    }

    if (rgbir_remosaic_reg->bypass == 0)
    {
        rgbir_remosaic_hw_core(tmp, out0_ptr, out1_ptr, xsize, ysize, bayer_pattern, rgbir_remosaic_reg);
    }
    else{
        rgbir_remosaic_hw_core_blinear(tmp, out0_ptr, out1_ptr, xsize, ysize, bayer_pattern, rgbir_remosaic_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 12);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 12);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void rgbir_remosaic_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

rgbir_remosaic_hw::~rgbir_remosaic_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (rgbir_remosaic_reg != nullptr)
    {
        delete rgbir_remosaic_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void rgbir_remosaic_hw::checkparameters(rgbir_remosaic_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");

    log_info("================= rgbir_remosaic reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("================= rgbir_remosaic reg=================\n");
}