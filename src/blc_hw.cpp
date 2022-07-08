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

static void blc_hw_core(uint16_t* indata, uint16_t* outdata, uint32_t xsize, uint32_t ysize, bayer_type_t by, const blc_reg_t* blc_reg)
{
    int32_t blc_val[4];
    if (by == RGGB)
    {
        blc_val[0] = blc_reg->blc_r;
        blc_val[1] = blc_reg->blc_gr;
        blc_val[2] = blc_reg->blc_gb;
        blc_val[3] = blc_reg->blc_b;
    }
    else if (by == GRBG)
    {
        blc_val[0] = blc_reg->blc_gr;
        blc_val[1] = blc_reg->blc_r;
        blc_val[2] = blc_reg->blc_b;
        blc_val[3] = blc_reg->blc_gb;
    }
    else if (by == GBRG)
    {
        blc_val[0] = blc_reg->blc_gb;
        blc_val[1] = blc_reg->blc_b;
        blc_val[2] = blc_reg->blc_r;
        blc_val[3] = blc_reg->blc_gr;
    }
    else if (by == BGGR)
    {
        blc_val[0] = blc_reg->blc_b;
        blc_val[1] = blc_reg->blc_gb;
        blc_val[2] = blc_reg->blc_gr;
        blc_val[3] = blc_reg->blc_r;
    }

    uint32_t white_level = blc_reg->white_level;

    uint32_t nom_ratio = (16383 * 1024) / white_level;

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        int32_t pix = indata[sz];
        uint32_t channel = ((ysize % 2) << 1) | (xsize % 2);
        if (channel > 3)
        {
            log_error("error for blc pix bayer select\n");
            channel = 3;
        }
        int32_t offset = blc_val[channel];

        pix = pix - offset;
        pix = (pix < 0) ? 0 : ((pix > 16383) ? 16383 : pix);

        if (blc_reg->normalize_en)
        {
            
            pix = (pix * nom_ratio + 512) >> 10;
            pix = (pix < 0) ? 0 : ((pix > 16383) ? 16383 : pix);
        }

        outdata[sz] = (uint16_t)pix;
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
        blc_reg->white_level = white_level;
        blc_reg->normalize_en = normalize_en;
    }
    else{
        blc_r =  blc_reg->blc_r;
        blc_gr =  blc_reg->blc_gr;
        blc_gb =  blc_reg->blc_gb;
        blc_b =  blc_reg->blc_b;
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
    reg->normalize_en = common_check_bits(reg->normalize_en, 1, "normalize_en");
    reg->white_level = common_check_bits(reg->white_level, 14, "white_level");

    log_info("================= blc reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("blc_r %d\n", reg->blc_r);
    log_info("blc_gr %d\n", reg->blc_gr);
    log_info("blc_gb %d\n", reg->blc_gb);
    log_info("blc_b %d\n", reg->blc_b);
    log_info("normalize_en %d\n", reg->normalize_en);
    log_info("white_level %d\n", reg->white_level);
    log_info("================= blc reg=================\n");
}