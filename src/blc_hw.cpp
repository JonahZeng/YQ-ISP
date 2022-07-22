#include "pipe_register.h"

blc_hw::blc_hw(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    blc_r = 0;
    blc_gr = 0;
    blc_gb = 0;
    blc_b = 0;

    blc_reg = new blc_reg_t;
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
    else if(by == IRGGR)
    {
        type[0] = PIX_Y; type[1] = PIX_GB; type[2] = PIX_Y; type[3] = PIX_GR;
        type[4] = PIX_GR; type[5] = PIX_R; type[6] = PIX_GB; type[7] = PIX_B;
        type[8] = PIX_Y; type[9] = PIX_GR; type[10] = PIX_Y; type[11] = PIX_GB;
        type[12] = PIX_GB; type[13] = PIX_B; type[14] = PIX_GR; type[15] = PIX_R;
    }
    else if(by == GIRBG)
    {
        type[0] = PIX_GR; type[1] = PIX_Y; type[2] = PIX_GB; type[3] = PIX_Y;
        type[4] = PIX_B; type[5] = PIX_GB; type[6] = PIX_R; type[7] = PIX_GR;
        type[8] = PIX_GB; type[9] = PIX_Y; type[10] = PIX_GR; type[11] = PIX_Y;
        type[12] = PIX_R; type[13] = PIX_GR; type[14] = PIX_B; type[15] = PIX_GB;
    }
    else if(by == IRGGB)
    {
        type[0] = PIX_Y; type[1] = PIX_GR; type[2] = PIX_Y; type[3] = PIX_GB;
        type[4] = PIX_GB; type[5] = PIX_B; type[6] = PIX_GR; type[7] = PIX_R;
        type[8] = PIX_Y; type[9] = PIX_GB; type[10] = PIX_Y; type[11] = PIX_GR;
        type[12] = PIX_GR; type[13] = PIX_R; type[14] = PIX_GB; type[15] = PIX_B;
    }
    else if(by == GIRRG)
    {
        type[0] = PIX_GB; type[1] = PIX_Y; type[2] = PIX_GR; type[3] = PIX_Y;
        type[4] = PIX_R; type[5] = PIX_GR; type[6] = PIX_B; type[7] = PIX_GB;
        type[8] = PIX_GR; type[9] = PIX_Y; type[10] = PIX_GB; type[11] = PIX_Y;
        type[12] = PIX_B; type[13] = PIX_GB; type[14] = PIX_R; type[15] = PIX_GR;
    }

    return type[pos];
}

static void blc_hw_core(uint16_t* indata, uint16_t* outdata, uint32_t xsize, uint32_t ysize, bayer_type_t by, const blc_reg_t* blc_reg)
{
    bool is_ir_sensor = false;
    if (by == RGGB || by == GRBG || by == GBRG || by == BGGR)
    {
        is_ir_sensor = false;
    }
    else if(by == RGGIR || by == GRIRG || by == BGGIR || by == GBIRG ||
        by == IRGGR || by == GIRRG || by == IRGGB || by == GIRBG)
    {
        is_ir_sensor = true;
    }


    uint32_t white_level = blc_reg->white_level;

    uint32_t nom_ratio = (16383 * 1024) / white_level;

    for (uint32_t row = 0; row < ysize; row++)
    {
        for(uint32_t col = 0; col < xsize; col++)
        {
            int32_t pix = indata[row * xsize + col];
            pixel_bayer_type pix_type;
            if (is_ir_sensor)
            {
                pix_type = get_pixel_bayer_type_ir(row, col, by);
            }
            else
            {
                pix_type = get_pixel_bayer_type(row, col, by);
            }
            int32_t offset = 0;
            if (pix_type == PIX_R)
            {
                offset = blc_reg->blc_r;
            }
            else if (pix_type == PIX_GR)
            {
                offset = blc_reg->blc_gr;
            }
            else if (pix_type == PIX_GB)
            {
                offset = blc_reg->blc_gb;
            }
            else if (pix_type == PIX_B)
            {
                offset = blc_reg->blc_b;
            }
            else if (pix_type == PIX_Y)
            {
                offset = blc_reg->blc_y;
            }

            pix = pix - offset;
            pix = (pix < 0) ? 0 : ((pix > 16383) ? 16383 : pix);

            if (blc_reg->normalize_en)
            {

                pix = (pix * nom_ratio + 512) >> 10;
                pix = (pix < 0) ? 0 : ((pix > 16383) ? 16383 : pix);
            }

            outdata[row * xsize + col] = (uint16_t)pix;
        }
    }
}

void blc_hw::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt) //input 2pins, 0~raw, 1~reg
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    bayer_type_t bayer_pattern = input_raw->bayer_pattern;

    if (in.size() > 1)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
        memcpy(blc_reg, &fe_reg->blc_reg, sizeof(blc_reg_t));
    }

    blc_reg->bypass = bypass;
    if (xmlConfigValid)
    {
        blc_reg->blc_r = blc_r;
        blc_reg->blc_gr = blc_gr;
        blc_reg->blc_gb = blc_gb;
        blc_reg->blc_b = blc_b;
        blc_reg->blc_y = blc_y;
        blc_reg->white_level = white_level;
        blc_reg->normalize_en = normalize_en;
    }
    else{
        blc_r =  blc_reg->blc_r;
        blc_gr =  blc_reg->blc_gr;
        blc_gb =  blc_reg->blc_gb;
        blc_b =  blc_reg->blc_b;
        blc_y =  blc_reg->blc_y;
        white_level =  blc_reg->white_level;
        normalize_en =  blc_reg->normalize_en;
    }

    checkparameters(blc_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, "blc_out0");
    uint16_t* out0_ptr = output0->data_ptr;
    out[0] = output0;

    std::unique_ptr<uint16_t[]> tmp_ptr(new uint16_t[xsize*ysize]);
    uint16_t* tmp = tmp_ptr.get();

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 14);
        out0_ptr[sz] = tmp[sz];
    }

    if (blc_reg->bypass == 0)
    {
        blc_hw_core(tmp, out0_ptr, xsize, ysize, bayer_pattern, blc_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14);
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void blc_hw::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          },
        {"blc_r",                  INT_32,      &this->blc_r           },
        {"blc_gr",                 INT_32,      &this->blc_gr          },
        {"blc_gb",                 INT_32,      &this->blc_gb          },
        {"blc_b",                  INT_32,      &this->blc_b           },
        {"blc_y",                  INT_32,      &this->blc_y           },
        {"normalize_en",           UINT_32,     &this->normalize_en    },
        {"white_level",            UINT_32,     &this->white_level     }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

blc_hw::~blc_hw()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (blc_reg != nullptr)
    {
        delete blc_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}

void blc_hw::checkparameters(blc_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->blc_r = common_check_bits_ex(reg->blc_r, 15, "blc_r");
    reg->blc_gr = common_check_bits_ex(reg->blc_gr, 15, "blc_gr");
    reg->blc_gb = common_check_bits_ex(reg->blc_gb, 15, "blc_gb");
    reg->blc_b = common_check_bits_ex(reg->blc_b, 15, "blc_b");
    reg->blc_y = common_check_bits_ex(reg->blc_y, 15, "blc_y");
    reg->normalize_en = common_check_bits(reg->normalize_en, 1, "normalize_en");
    reg->white_level = common_check_bits(reg->white_level, 14, "white_level");

    log_info("================= blc reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("blc_r %d\n", reg->blc_r);
    log_info("blc_gr %d\n", reg->blc_gr);
    log_info("blc_gb %d\n", reg->blc_gb);
    log_info("blc_b %d\n", reg->blc_b);
    log_info("blc_y %d\n", reg->blc_y);
    log_info("normalize_en %d\n", reg->normalize_en);
    log_info("white_level %d\n", reg->white_level);
    log_info("================= blc reg=================\n");
}