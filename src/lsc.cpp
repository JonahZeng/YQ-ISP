#include "fe_firmware.h"

lsc::lsc(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name),luma_gain(LSC_GRID_ROWS * LSC_GRID_COLS)
{
    bypass = 0;
    block_size_x = 152;
    block_size_y = 136;

    block_start_x_idx = 0;
    block_start_x_oft = 0;

    block_start_y_idx = 0;
    block_start_y_oft = 0;
    lsc_reg = new lsc_reg_t;
}

static void lsc_hw_core(uint16_t* indata, uint16_t* outdata, uint32_t xsize, uint32_t ysize, const lsc_reg_t* lsc_reg)
{
    uint32_t x_grad = (1U << 15) / lsc_reg->block_size_x;
    uint32_t y_grad = (1U << 15) / lsc_reg->block_size_y;
    uint32_t y_block = lsc_reg->block_start_y_idx;
    uint32_t y_offset = lsc_reg->block_start_y_oft;
    for (uint32_t y = 0; y < ysize; y++)
    {
        if (y_offset >= lsc_reg->block_size_y)
        {
            y_block += 1;
            y_block = (y_block > (LSC_GRID_ROWS - 2)) ? (LSC_GRID_ROWS - 2) : y_block; //max y block 31-2 29
            y_offset = 0;
        }
        uint32_t x_block = lsc_reg->block_start_x_idx;
        uint32_t x_offset = lsc_reg->block_start_x_oft;
        for (uint32_t x = 0; x < xsize; x++)
        {
            if (x_offset >= lsc_reg->block_size_x)
            {
                x_block += 1;
                x_block = (x_block > (LSC_GRID_COLS - 2)) ? (LSC_GRID_COLS - 2) : x_block;
                x_offset = 0;
            }

            int32_t gain_lt = (int32_t)lsc_reg->luma_gain[y_block*LSC_GRID_COLS + x_block];
            int32_t gain_rt = (int32_t)lsc_reg->luma_gain[y_block*LSC_GRID_COLS + x_block + 1];
            int32_t gain_lb = (int32_t)lsc_reg->luma_gain[(y_block + 1)*LSC_GRID_COLS + x_block];
            int32_t gain_rb = (int32_t)lsc_reg->luma_gain[(y_block + 1)*LSC_GRID_COLS + x_block + 1];
            //bilinear interpolation

            int64_t gain0 = gain_lt + (((int64_t)(gain_rt - gain_lt) * x_offset * x_grad + 16383) >> 15);
            int64_t gain1 = gain_lb + (((int64_t)(gain_rb - gain_lb) * x_offset * x_grad + 16383) >> 15);
            int64_t gain = gain0 + (((int64_t)(gain1 - gain0) * y_offset * y_grad + 16383) >> 15);


            uint32_t tmp = (indata[y*xsize + x] * (uint32_t)gain + 512) >> 10;
            tmp = (tmp > 16383) ? 16383 : tmp;
            outdata[y*xsize + x] = (uint16_t)tmp;

            x_offset++;
        }
        y_offset++;
    }
}

void lsc::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_raw = in[0];
    //bayer_type_t bayer_pattern = input_raw->bayer_pattern;
    if (in.size() > 1)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[1]->data_ptr);
        memcpy(lsc_reg, &fe_reg->lsc_reg, sizeof(lsc_reg_t));
    }

    lsc_reg->bypass = bypass;

    if (xmlConfigValid)
    {
        lsc_reg->block_size_x = block_size_x;
        lsc_reg->block_size_y = block_size_y;
        lsc_reg->block_start_x_idx = block_start_x_idx;
        lsc_reg->block_start_x_oft = block_start_x_oft;
        lsc_reg->block_start_y_oft = block_start_y_oft;
        lsc_reg->block_start_y_oft = block_start_y_oft;

        for (int32_t i = 0; i < LSC_GRID_ROWS * LSC_GRID_COLS; i++)
        {
            lsc_reg->luma_gain[i] = luma_gain[i];
        }
    }

    checkparameters(lsc_reg);

    uint32_t xsize = input_raw->width;
    uint32_t ysize = input_raw->height;

    data_buffer* output0 = new data_buffer(xsize, ysize, input_raw->data_type, input_raw->bayer_pattern, "lsc_out0");
    uint16_t* out0_ptr = output0->data_ptr;
    out[0] = output0;

    uint16_t* tmp = new uint16_t[xsize*ysize];

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp[sz] = input_raw->data_ptr[sz] >> (16 - 14); //nikon d610 14bit capture raw, right shift to 14bit precision
        out0_ptr[sz] = tmp[sz];
    }

    if (lsc_reg->bypass == 0)
    {
        lsc_hw_core(tmp, out0_ptr, xsize, ysize, lsc_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 14); //nikon d610 14bit capture raw, back to high bits
    }

    delete[] tmp;

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void lsc::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,      &this->bypass                 },
        {"block_size_x",           UINT_32,      &this->block_size_x           },
        {"block_size_y",           UINT_32,      &this->block_size_y           },
        {"block_start_x_idx",      UINT_32,      &this->block_start_x_idx      },
        {"block_start_x_oft",      UINT_32,      &this->block_start_x_oft      },
        {"block_start_y_idx",      UINT_32,      &this->block_start_y_idx      },
        {"block_start_y_oft",      UINT_32,      &this->block_start_y_oft      },
        {"luma_gain",              VECT_UINT32,  &this->luma_gain,         LSC_GRID_ROWS * LSC_GRID_COLS}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s init run end\n", name);
}

lsc::~lsc()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (lsc_reg != NULL)
    {
        delete lsc_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}


void lsc::checkparameters(lsc_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");
    reg->block_size_x = common_check_bits(reg->block_size_x, 8, "block_size_x");
    reg->block_size_y = common_check_bits(reg->block_size_y, 8, "block_size_y");
    reg->block_start_x_idx = common_check_bits(reg->block_start_x_idx, 6, "block_start_x_idx");
    reg->block_start_x_oft = common_check_bits(reg->block_start_x_oft, 8, "block_start_x_oft");
    reg->block_start_y_idx = common_check_bits(reg->block_start_y_idx, 5, "block_start_y_idx");
    reg->block_start_y_oft = common_check_bits(reg->block_start_y_oft, 8, "block_start_y_oft");
    for (int32_t i = 0; i < LSC_GRID_ROWS * LSC_GRID_COLS; i++)
    {
        reg->luma_gain[i] = common_check_bits(reg->luma_gain[i], 14, "luma_gain"); //4.10bit
    }

    log_info("================= lsc reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("block_size_x %d\n", reg->block_size_x);
    log_info("block_size_y %d\n", reg->block_size_y);
    log_info("block_start_x_idx %d\n", reg->block_start_x_idx);
    log_info("block_start_x_oft %d\n", reg->block_start_x_oft);
    log_info("block_start_y_idx %d\n", reg->block_start_y_idx);
    log_info("block_start_y_oft %d\n", reg->block_start_y_oft);
    log_array("luma gain:\n", "%5d, ", reg->luma_gain, LSC_GRID_ROWS * LSC_GRID_COLS, LSC_GRID_COLS);
    log_info("================= lsc reg=================\n");
}
