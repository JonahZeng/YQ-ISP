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

#include "meta_data.h"

extern dng_md_t dng_all_md;

fileRead::fileRead(uint32_t outpins, const char* inst_name):hw_base(0, outpins, inst_name), file_name()
{
    bayer = RGGB;
    bit_depth = 10;
    file_type = RAW_FORMAT;
    img_width = 0;
    img_height = 0;
}


static void get_blc_md_from_dng(uint32_t blc_dim_x, uint32_t blc_dim_y, AutoPtr<dng_negative>& negative, bayer_type_t bayer_tp, dng_md_t& all_md)
{
    if (blc_dim_x == 1 && blc_dim_y == 1)
    {
        all_md.blc_md.white_level = (int32_t)negative->GetLinearizationInfo()->fWhiteLevel[0];

        all_md.blc_md.r_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
        all_md.blc_md.gr_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
        all_md.blc_md.gb_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
        all_md.blc_md.b_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
    }
    else if (blc_dim_x == 2 && blc_dim_y == 2)
    {
        all_md.blc_md.white_level = (int32_t)negative->GetLinearizationInfo()->fWhiteLevel[0];

        if (bayer_tp == RGGB)
        {
            all_md.blc_md.r_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
            all_md.blc_md.gr_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][1][0];
            all_md.blc_md.gb_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][0][0];
            all_md.blc_md.b_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][1][0];
        }
        else if (bayer_tp == GRBG)
        {
            all_md.blc_md.gr_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
            all_md.blc_md.r_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][1][0];
            all_md.blc_md.b_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][0][0];
            all_md.blc_md.gb_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][1][0];
        }
        else if (bayer_tp == GBRG)
        {
            all_md.blc_md.gb_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
            all_md.blc_md.b_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][1][0];
            all_md.blc_md.r_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][0][0];
            all_md.blc_md.gr_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][1][0];
        }
        else {
            all_md.blc_md.b_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][0][0];
            all_md.blc_md.gb_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[0][1][0];
            all_md.blc_md.gr_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][0][0];
            all_md.blc_md.r_black_level = (int32_t)negative->GetLinearizationInfo()->fBlackLevel[1][1][0];
        }
    }
    else {
        spdlog::error("blc channel must be 1 or 4");
        exit(1);
    }
}

static void get_lsc_md_from_dng(dng_exif* exif, dng_ifd* main_ifd, dng_md_t& all_md)
{
    /*struct dng_name_table
    {
        uint32 key;
        const char *name;
    };

    enum
    {

        ruNone = 1,
        ruInch = 2,
        ruCM = 3,
        ruMM = 4,
        ruMicroM = 5

    };

    const dng_name_table kResolutionUnitNames[] =
    {
    {	ruNone, 	"None"			},
    {	ruInch,		"Inch"			},
    {	ruCM,		"cm"			},
    {	ruMM,		"mm"			},
    {	ruMicroM,	"Micrometer"	}
    };*/


    all_md.lsc_md.FocalLength = exif->fFocalLength.As_real64();
    all_md.lsc_md.FocalPlaneResolutionUnit = exif->fFocalPlaneResolutionUnit;
    all_md.lsc_md.FocalPlaneXResolution = exif->fFocalPlaneXResolution.As_real64();
    all_md.lsc_md.FocalPlaneYResolution = exif->fFocalPlaneYResolution.As_real64();
    all_md.lsc_md.FocusDistance = exif->fApproxFocusDistance.As_real64();

    all_md.lsc_md.img_w = main_ifd->fImageWidth;
    all_md.lsc_md.img_h = main_ifd->fImageLength;

    all_md.lsc_md.crop_x = 0; //current pipeline no crop
    all_md.lsc_md.crop_y = 0;

    spdlog::info("lsc info: FocalLength {}mm, FocalPlaneResolutionUnit {} 1:none, 2:inch, 3:cm, 4:mm, 5:microm, \nFocalPlaneXResolution {}, FocalPlaneYResolution {}, FocusDistance {}m",
        all_md.lsc_md.FocalLength, all_md.lsc_md.FocalPlaneResolutionUnit, all_md.lsc_md.FocalPlaneXResolution, 
        all_md.lsc_md.FocalPlaneYResolution, all_md.lsc_md.FocusDistance);
    if (all_md.lsc_md.FocalPlaneResolutionUnit == 1)
    {
        spdlog::error("no ResolutionUnit");
        exit(1);
    }
}

static void get_ae_md_from_dng(dng_exif* exif, dng_md_t& all_md)
{
    all_md.ae_md.ApertureValue = exif->fApertureValue.As_real64();
    all_md.ae_md.ISOSpeedRatings[0] = exif->fISOSpeedRatings[0];
    all_md.ae_md.ISOSpeedRatings[1] = exif->fISOSpeedRatings[1];
    all_md.ae_md.ISOSpeedRatings[2] = exif->fISOSpeedRatings[2];
    all_md.ae_md.ShutterSpeedValue = exif->fShutterSpeedValue.As_real64();

    spdlog::info("ae info: apertureValue {}, ISO {} {} {}, shutterSpeedValue {}s", 
        all_md.ae_md.ApertureValue, all_md.ae_md.ISOSpeedRatings[0], all_md.ae_md.ISOSpeedRatings[1], 
        all_md.ae_md.ISOSpeedRatings[2], all_md.ae_md.ShutterSpeedValue);
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
    spdlog::info("total size {}, {}", stage1->Width(), stage1->Height());

    if (*out0 == nullptr)
    {
        *out0 = new data_buffer(stage1->Width(), stage1->Height(), RAW, bayer_tp, "raw_in out0");
    }
    void* p_raw = (*out0)->data_ptr;
    dng_pixel_buffer raw_buf(dng_rect(stage1->Height(), stage1->Width()), 0, stage1->Planes(), stage1->PixelType(), pcPlanar, p_raw);
    stage1->Get(raw_buf);
    negative->SynchronizeMetadata(); //xmp exif info include focus distance
    dng_exif* exif = negative->GetExif();

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
    int32_t t = negative->GetLinearizationInfo()->fActiveArea.t;
    int32_t l = negative->GetLinearizationInfo()->fActiveArea.l;
    int32_t b = negative->GetLinearizationInfo()->fActiveArea.b;
    int32_t r = negative->GetLinearizationInfo()->fActiveArea.r;

    spdlog::info("active region {}, {}, {}, {}", l, t, r, b);

    uint32_t blc_dim_x = negative->GetLinearizationInfo()->fBlackLevelRepeatCols;
    uint32_t blc_dim_y = negative->GetLinearizationInfo()->fBlackLevelRepeatRows;

    spdlog::info("black level repeat dim {}, {}, {} value:", blc_dim_x, blc_dim_y, stage1->Planes());
    for (int i = 0; i < kMaxBlackPattern; i++)
    {
        for (int j = 0; j < kMaxBlackPattern; j++)
        {
            spdlog::info("{}, {}, {}, {}", negative->GetLinearizationInfo()->fBlackLevel[i][j][0],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][1],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][2],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][3]);
        }
    }

    spdlog::info("bit depth is {} white_level = {} {} {} {}", *bit_depth, negative->GetLinearizationInfo()->fWhiteLevel[0],
        negative->GetLinearizationInfo()->fWhiteLevel[1],
        negative->GetLinearizationInfo()->fWhiteLevel[2],
        negative->GetLinearizationInfo()->fWhiteLevel[3]);

    get_blc_md_from_dng(blc_dim_x, blc_dim_y, negative, bayer_tp, dng_all_md);
    get_ae_md_from_dng(exif, dng_all_md);
    get_lsc_md_from_dng(exif, info.fIFD[info.fMainIndex], dng_all_md);

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
#ifdef _MSC_VER
        fopen_s(&input_f, file_name, "rb");
#else
        input_f = fopen(file_name, "rb");
#endif
        if (input_f != nullptr)
        {
            if (bit_depth > 8 && bit_depth <= 16)
            {
                size_t read_cnt = fread(out0->data_ptr, sizeof(uint16_t), img_width*img_height, input_f);
                if (read_cnt != sizeof(uint16_t) * img_width * img_height)
                {
                    spdlog::error("read raw file bytes unexpected");
                }
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

    hw_base::init();
    spdlog::info("{0} run end", __FUNCTION__);
}

fileRead::~fileRead()
{
    spdlog::info("{0} deinit start", __FUNCTION__);
    spdlog::info("{0} deinit end", __FUNCTION__);
}