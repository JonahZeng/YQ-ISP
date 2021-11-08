#include <assert.h>
#include "fileRead.h"

//#include "dng_color_space.h"
#include "dng_camera_profile.h"
//#include "dng_date_time.h"
//#include "dng_exceptions.h"
#include "dng_file_stream.h"
//#include "dng_globals.h"
#include "dng_host.h"
#include "dng_ifd.h"
//#include "dng_image_writer.h"
#include "dng_info.h"
//#include "dng_linearization_info.h"
#include "dng_mosaic_info.h"
#include "dng_negative.h"
//#include "dng_preview.h"
//#include "dng_render.h"
//#include "dng_simple_image.h"
//#include "dng_tag_codes.h"
//#include "dng_tag_types.h"
//#include "dng_tag_values.h"
//#include "dng_xmp.h"
//#include "dng_xmp_sdk.h"

#include "meta_data.h"

extern dng_md_t g_dng_all_md;

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
        log_error("blc channel must be 1 or 4\n");
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

    all_md.lsc_md.crop_x = (uint32_t)(main_ifd->fDefaultCropOriginH.As_real64());
    all_md.lsc_md.crop_y = (uint32_t)(main_ifd->fDefaultCropOriginV.As_real64());

    log_info("lsc info: FocalLength %lf mm, FocalPlaneResolutionUnit %d 1:none, 2:inch, 3:cm, 4:mm, 5:microm\n",
        all_md.lsc_md.FocalLength, all_md.lsc_md.FocalPlaneResolutionUnit);
    log_info("FocalPlaneXResolution %lf, \
        FocalPlaneYResolution %lf, FocusDistance %lf m\n", all_md.lsc_md.FocalPlaneXResolution,
        all_md.lsc_md.FocalPlaneYResolution, all_md.lsc_md.FocusDistance);
    if (all_md.lsc_md.FocalPlaneResolutionUnit == 1)
    {
        log_error("no ResolutionUnit\n");
        exit(1);
    }
}

static void get_ae_md_from_dng(dng_exif* exif, dng_md_t& all_md)
{
    all_md.ae_md.ApertureValue = exif->fApertureValue.As_real64();
    all_md.ae_md.ISOSpeedRatings[0] = exif->fISOSpeedRatings[0];
    all_md.ae_md.ISOSpeedRatings[1] = exif->fISOSpeedRatings[1];
    all_md.ae_md.ISOSpeedRatings[2] = exif->fISOSpeedRatings[2];
    all_md.ae_md.ExposureTime = exif->fExposureTime.As_real64();
    double tmp = (1.0 / all_md.ae_md.ApertureValue) * (1.0 / all_md.ae_md.ApertureValue) / all_md.ae_md.ExposureTime;
    double Ev = log2(tmp);
    double Sv = log2(0.297*all_md.ae_md.ISOSpeedRatings[0]);
    all_md.ae_md.BrightnessValue = Ev - Sv;

    log_info("ae info: apertureValue F 1/%lf, ISO %d %d %d, ExposureTime %lf s, BV %.4lf\n", 
        all_md.ae_md.ApertureValue, all_md.ae_md.ISOSpeedRatings[0], all_md.ae_md.ISOSpeedRatings[1], 
        all_md.ae_md.ISOSpeedRatings[2], all_md.ae_md.ExposureTime, all_md.ae_md.BrightnessValue);
    
}

static void get_awb_md_from_dng(dng_info* info, dng_md_t& all_md)
{
    if (info->fShared->fAsShotNeutral.Count() != 3)
    {
        log_error("img planes != 3\n");
        exit(1);
    }

    all_md.awb_md.r_Neutral = info->fShared->fAsShotNeutral[0];
    all_md.awb_md.g_Neutral = info->fShared->fAsShotNeutral[1];
    all_md.awb_md.b_Neutral = info->fShared->fAsShotNeutral[2];

    all_md.awb_md.asShot_valid = info->fShared->fAsShotWhiteXY.IsValid();
    all_md.awb_md.asShot_x = info->fShared->fAsShotWhiteXY.x;
    all_md.awb_md.asShot_y = info->fShared->fAsShotWhiteXY.y;
}

static void get_cc_md_from_dng(dng_info* info, dng_md_t& all_md)
{
    if (info->fShared->fAnalogBalance.Count() != 3)
    {
        log_error("img planes != 3\n");
        exit(1);
    }
    all_md.cc_md.Analogbalance[0] = info->fShared->fAnalogBalance[0];
    all_md.cc_md.Analogbalance[1] = info->fShared->fAnalogBalance[1];
    all_md.cc_md.Analogbalance[2] = info->fShared->fAnalogBalance[2];

    all_md.cc_md.CalibrationIlluminant1 = info->fShared->fCameraProfile.fCalibrationIlluminant1;
    all_md.cc_md.CalibrationIlluminant2 = info->fShared->fCameraProfile.fCalibrationIlluminant2;

    if (info->fShared->fCameraProfile.fColorMatrix1.Rows() != 3 || info->fShared->fCameraProfile.fColorMatrix1.Cols() != 3)
    {
        log_error("CM1 dim != 3*3\n");
        exit(1);
    }
    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.ColorMatrix1[row][col] = info->fShared->fCameraProfile.fColorMatrix1[row][col];
        }
    }
    if (info->fShared->fCameraProfile.fColorMatrix2.Rows() != 3 || info->fShared->fCameraProfile.fColorMatrix2.Cols() != 3)
    {
        log_error("CM2 dim != 3*3\n");
        exit(1);
    }
    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.ColorMatrix2[row][col] = info->fShared->fCameraProfile.fColorMatrix2[row][col];
        }
    }
    if (info->fShared->fCameraCalibration1.Rows() != 3 || info->fShared->fCameraCalibration1.Cols() != 3)
    {
        log_error("CC1 dim != 3*3\n");
        exit(1);
    }
    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.CameraCalibration1[row][col] = info->fShared->fCameraCalibration1[row][col];
        }
    }
    if (info->fShared->fCameraCalibration2.Rows() != 3 || info->fShared->fCameraCalibration2.Cols() != 3)
    {
        log_error("CC2 dim != 3*3\n");
        exit(1);
    }

    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.CameraCalibration2[row][col] = info->fShared->fCameraCalibration2[row][col];
        }
    }

    if (info->fShared->fCameraProfile.fForwardMatrix1.Rows() != 3 || info->fShared->fCameraProfile.fForwardMatrix1.Cols() != 3)
    {
        log_error("FM1 dim != 3*3\n");
        exit(1);
    }
    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.ForwardMatrix1[row][col] = info->fShared->fCameraProfile.fForwardMatrix1[row][col];
        }
    }

    if (info->fShared->fCameraProfile.fForwardMatrix2.Rows() != 3 || info->fShared->fCameraProfile.fForwardMatrix2.Cols() != 3)
    {
        log_error("FM2 dim != 3*3\n");
        exit(1);
    }
    for (uint32_t row = 0; row < 3; row++)
    {
        for (uint32_t col = 0; col < 3; col++)
        {
            all_md.cc_md.ForwardMatrix2[row][col] = info->fShared->fCameraProfile.fForwardMatrix2[row][col];
        }
    }
}

static void get_sensor_crop_info_from_dng(dng_info* info, dng_md_t& all_md)
{
    all_md.sensor_crop_size_info.origin_x = (uint32_t)(info->fIFD[info->fMainIndex]->fDefaultCropOriginH.As_real64());
    all_md.sensor_crop_size_info.origin_y = (uint32_t)(info->fIFD[info->fMainIndex]->fDefaultCropOriginV.As_real64());
    all_md.sensor_crop_size_info.width = (uint32_t)(info->fIFD[info->fMainIndex]->fDefaultCropSizeH.As_real64());
    all_md.sensor_crop_size_info.height = (uint32_t)(info->fIFD[info->fMainIndex]->fDefaultCropSizeV.As_real64());
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

    log_info("bayer: %d 0:RGGB 1:GRBG 2:GBRG 3:BGGR\n", bayer_tp);

    const dng_image* stage1 = negative->Stage1Image();
    if (stage1->PixelSize() != 2)
    {
        log_error("pixel size must be 2 byte\n");
        return;
    }
    log_info("total size %d, %d\n", stage1->Width(), stage1->Height());

    if (*out0 == nullptr)
    {
        *out0 = new data_buffer(stage1->Width(), stage1->Height(), DATA_TYPE_RAW, bayer_tp, "raw_in out0");
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
        log_warning("bit depth is 8");
        *bit_depth = 8;
    }
    int32_t t = negative->GetLinearizationInfo()->fActiveArea.t;
    int32_t l = negative->GetLinearizationInfo()->fActiveArea.l;
    int32_t b = negative->GetLinearizationInfo()->fActiveArea.b;
    int32_t r = negative->GetLinearizationInfo()->fActiveArea.r;

    log_info("active region %d, %d, %d, %d\n", l, t, r, b);

    uint32_t blc_dim_x = negative->GetLinearizationInfo()->fBlackLevelRepeatCols;
    uint32_t blc_dim_y = negative->GetLinearizationInfo()->fBlackLevelRepeatRows;

    log_info("black level repeat dim %d, %d, %d value:\n", blc_dim_x, blc_dim_y, stage1->Planes());
    for (int i = 0; i < kMaxBlackPattern; i++)
    {
        for (int j = 0; j < kMaxBlackPattern; j++)
        {
            log_info("%lf, %lf, %lf, %lf\n", negative->GetLinearizationInfo()->fBlackLevel[i][j][0],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][1],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][2],
                negative->GetLinearizationInfo()->fBlackLevel[i][j][3]);
        }
    }

    log_info("bit depth is %d white_level = %lf, %lf, %lf, %lf\n", *bit_depth, negative->GetLinearizationInfo()->fWhiteLevel[0],
        negative->GetLinearizationInfo()->fWhiteLevel[1],
        negative->GetLinearizationInfo()->fWhiteLevel[2],
        negative->GetLinearizationInfo()->fWhiteLevel[3]);

    get_blc_md_from_dng(blc_dim_x, blc_dim_y, negative, bayer_tp, g_dng_all_md);

    get_ae_md_from_dng(exif, g_dng_all_md);
    

    g_dng_all_md.ae_comp_md.BaselineExposure = info.fShared->fBaselineExposure.As_real64();
    log_info("baseline exposure = %lf\n", g_dng_all_md.ae_comp_md.BaselineExposure);
    
    get_awb_md_from_dng(&info, g_dng_all_md);

    get_cc_md_from_dng(&info, g_dng_all_md);

    get_sensor_crop_info_from_dng(&info, g_dng_all_md);

    get_lsc_md_from_dng(exif, info.fIFD[info.fMainIndex], g_dng_all_md);

    log_info("tone curve:\n");
    uint32_t profile_cnt = negative->ProfileCount();
    for (uint32_t i = 0; i < profile_cnt; i++)
    {
        const dng_camera_profile& pf= negative->ProfileByIndex(i);
        const dng_tone_curve& curve = pf.ToneCurve();
        size_t sz = curve.fCoord.size();
        for (size_t j = 0; j < sz; j++)
        {
            log_info("%lf, %lf\n", curve.fCoord[j].h, curve.fCoord[j].v);
        }
    }

}

void fileRead::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start frame %d\n", __FUNCTION__, frame_cnt);
    if (frame_cnt == 0)
    {
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
            data_buffer* out0 = new data_buffer(img_width, img_height, DATA_TYPE_RAW, raw_bayer, "raw_in out0");
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
                        log_error("read raw file bytes unexpected\n");
                    }
                    for (uint32_t s = 0; s < img_height*img_width; s++)
                    {
                        out0->data_ptr[s] = out0->data_ptr[s] << (16 - bit_depth);
                    }
                }
                fclose(input_f);
                g_dng_all_md.input_file_name = std::string(file_name);
            }
        }
        else if (strcmp(file_type_string, "DNG") == 0)
        {
            data_buffer* out0 = nullptr;
            uint32_t bit_dep = 0;
            readDNG_by_adobe_sdk(file_name, &out0, &bit_dep);
            g_dng_all_md.input_file_name = std::string(file_name);
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
            log_warning("{0} not support yet", file_type_string);
        }
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end frame %d\n", __FUNCTION__, frame_cnt);
}

void fileRead::init()
{
    log_info("%s init run start\n", name);
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
    log_info("%s init run end\n", name);
}

fileRead::~fileRead()
{
    log_info("%s deinit start\n", __FUNCTION__);
    log_info("%s deinit end\n", __FUNCTION__);
}