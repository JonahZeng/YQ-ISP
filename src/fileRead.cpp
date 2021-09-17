#include "fileRead.h"

#include "dng_color_space.h"
#include "dng_date_time.h"
#include "dng_exceptions.h"
#include "dng_file_stream.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_ifd.h"
#include "dng_image_writer.h"
#include "dng_info.h"
#include "dng_linearization_info.h"
#include "dng_mosaic_info.h"
#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_render.h"
#include "dng_simple_image.h"
#include "dng_tag_codes.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_xmp.h"
#include "dng_xmp_sdk.h"

fileRead::fileRead(uint32_t outpins, const char* inst_name):hw_base(0, outpins, inst_name), file_name()
{
    bayer = RGGB;
    bit_depth = 10;
    file_type = RAW_FORMAT;
    img_width = 0;
    img_height = 0;
}


static void readDNG_by_adobe_sdk(char* file_name, data_buffer** out0, uint32_t* bit_depth)
{
    dng_file_stream stream(file_name);
    dng_host host;
    AutoPtr<dng_negative> negative;
    dng_info info;
    info.Parse(host, stream);
    info.PostParse(host);
    if (!info.IsValidDNG())
    {
        return;
    }

    negative.Reset(host.Make_dng_negative());
    negative->Parse(host, stream, info);
    negative->PostParse(host, stream, info);
    negative->ReadStage1Image(host, stream, info);
    if (info.fMaskIndex != -1)
    {
        negative->ReadStage1Image(host, stream, info);
    }
    negative->ValidateRawImageDigest(host);
    const dng_mosaic_info* mosaic_info = negative->GetMosaicInfo();
    int32_t xdim = mosaic_info->fCFAPatternSize.h;
    int32_t ydim = mosaic_info->fCFAPatternSize.v;

    bayer_type_t bayer_tp;
    assert(xdim == 2 && ydim == 2);
    if (mosaic_info->fCFAPattern[0][0] == 0 &&
        mosaic_info->fCFAPattern[0][1] == 1 &&
        mosaic_info->fCFAPattern[1][0] == 1 &&
        mosaic_info->fCFAPattern[1][1] == 2)
    {
        bayer_tp = RGGB;
    }
    else if (mosaic_info->fCFAPattern[0][0] == 1 &&
        mosaic_info->fCFAPattern[0][1] == 0 &&
        mosaic_info->fCFAPattern[1][0] == 2 &&
        mosaic_info->fCFAPattern[1][1] == 1)
    {
        bayer_tp = GRBG;
    }
    else if (mosaic_info->fCFAPattern[0][0] == 1 &&
        mosaic_info->fCFAPattern[0][1] == 2 &&
        mosaic_info->fCFAPattern[1][0] == 0 &&
        mosaic_info->fCFAPattern[1][1] == 1)
    {
        bayer_tp = GBRG;
    }
    else
    {
        bayer_tp = BGGR;
    }

    spdlog::info("bayer: {} 0:RGGB 1:GRBG 2:GBRG 3:BGGR", bayer_tp);

    const dng_image* stage1 = negative->Stage1Image();
    if (stage1->PixelSize() != 2)
    {
        spdlog::error("pixel size must be 2 byte");
        return;
    }

    if (*out0 == nullptr)
    {
        *out0 = new data_buffer(stage1->Width(), stage1->Height(), RAW, bayer_tp, "raw_in out0");
    }
    void* p_raw = (*out0)->data_ptr;
    dng_pixel_buffer raw_buf(dng_rect(stage1->Height(), stage1->Width()), 0, stage1->Planes(), stage1->PixelType(), pcPlanar, p_raw);
    stage1->Get(raw_buf);
    negative->SynchronizeMetadata();
    *bit_depth = info.fIFD[info.fMainIndex]->fBitsPerSample[0];

    uint32_t white_level = negative->WhiteLevel();
    if (white_level <= 65535 && white_level > 16383)
    {
        *bit_depth = 16;
    }
    else if (white_level <= 16383 && white_level > 4095)
    {
        *bit_depth = 14;
    }
    else if (white_level <= 4095 && white_level > 1023)
    {
        *bit_depth = 12;
    }
    else if (white_level <= 1023 && white_level > 255)
    {
        *bit_depth = 10;
    }
    else {
        spdlog::warn("bit depth is 8");
        *bit_depth = 8;
    }
    spdlog::info("bit depth is {}", *bit_depth);
}

void fileRead::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    spdlog::info("{0} run start", __FUNCTION__);
    if (strcmp(file_type_string, "RAW") == 0)
    {
        bayer_type_t raw_bayer;
        if (strcmp(this->bayer_string, "RGGB") == 0)
        {
            raw_bayer = RGGB;
        }
        else if (strcmp(this->bayer_string, "GRBG") == 0)
        {
            raw_bayer = GRBG;
        }
        else if (strcmp(this->bayer_string, "GBRG") == 0)
        {
            raw_bayer = GBRG;
        }
        else if (strcmp(this->bayer_string, "BGGR") == 0)
        {
            raw_bayer = BGGR;
        }
        data_buffer* out0 = new data_buffer(img_width, img_height, RAW, raw_bayer, "raw_in out0");
        out[0] = out0;

        FILE* input_f;
        fopen_s(&input_f, file_name, "rb");
        if (input_f != nullptr)
        {
            if (bit_depth > 8 && bit_depth <= 16)
            {
                fread(out0->data_ptr, sizeof(uint16_t), img_width*img_height, input_f);
                for (uint32_t s = 0; s < img_height*img_width; s++)
                {
                    out0->data_ptr[s] = out0->data_ptr[s] << (16 - bit_depth);
                }
            }
            fclose(input_f);
        }
    }
    else if (strcmp(file_type_string, "DNG") == 0)
    {
        data_buffer* out0 = nullptr;
        uint32_t bit_dep = 0;
        readDNG_by_adobe_sdk(file_name, &out0, &bit_dep);
        out[0] = out0;
        img_width = out0->width;
        img_height = out0->height;
        bit_depth = bit_dep;
        if (bit_depth > 8 && bit_depth <= 16)
        {
            for (uint32_t s = 0; s < img_height*img_width; s++)
            {
                out0->data_ptr[s] = out0->data_ptr[s] << (16 - bit_depth);
            }
        }
    }
    else {
        spdlog::warn("{0} not support yet", file_type_string);
    }
    spdlog::info("{0} run end", __FUNCTION__);
}

void fileRead::init()
{
    spdlog::info("{0} run start", __FUNCTION__);
    cfgEntry_t config[] = {
        {"bayer_type",     STRING,     this->bayer_string,      32},
        {"bit_depth",      UINT_32,    &this->bit_depth           },
        {"file_type",      STRING,     this->file_type_string,  32},
        {"file_name",      STRING,     this->file_name,        256},
        {"img_width",      UINT_32,    &this->img_width           },
        {"img_height",     UINT_32,    &this->img_height          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }
    spdlog::info("{0} run end", __FUNCTION__);
}

fileRead::~fileRead()
{
    spdlog::info("{0} deinit end", __FUNCTION__);
}