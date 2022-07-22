#include <assert.h>
#include "file_read.h"

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

// #include "meta_data.h"
#include "pipeline_manager.h"


file_read::file_read(uint32_t outpins, const char* inst_name):hw_base(0, outpins, inst_name), file_name()
{
    bayer = RGGB;
    bit_depth = 10;
    file_type = RAW_FORMAT;
    img_width = 0;
    img_height = 0;
    big_endian = false;
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
    double Av = 2 * log2(all_md.ae_md.ApertureValue);
    double Tv = -log2(all_md.ae_md.ExposureTime);
    double Ev = Av + Tv;
    double Sv = log2(all_md.ae_md.ISOSpeedRatings[0]/3.125);
    all_md.ae_md.Bv = Ev - Sv;

    log_info("ae info: apertureValue F 1/%lf, ISO %d %d %d, ExposureTime %lf s, BV %.4lf\n", 
        all_md.ae_md.ApertureValue, all_md.ae_md.ISOSpeedRatings[0], all_md.ae_md.ISOSpeedRatings[1],
        all_md.ae_md.ISOSpeedRatings[2], all_md.ae_md.ExposureTime, all_md.ae_md.Bv);
    
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

    //if (info.fMaskIndex != -1)
    //{
    //    negative->ReadStage1Image(host, stream, info);
    //}
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

    pipeline_manager* cur_pipe_manager = pipeline_manager::get_current_pipe_manager();
    dng_md_t& dng_all_md = cur_pipe_manager->global_ref_out->dng_meta_data;
    dng_all_md.meta_data_valid = true;

    get_blc_md_from_dng(blc_dim_x, blc_dim_y, negative, bayer_tp, dng_all_md);

    get_ae_md_from_dng(exif, dng_all_md);
    

    dng_all_md.ae_comp_md.BaselineExposure = info.fShared->fBaselineExposure.As_real64();
    log_info("baseline exposure = %lf\n", dng_all_md.ae_comp_md.BaselineExposure);
    
    get_awb_md_from_dng(&info, dng_all_md);

    get_cc_md_from_dng(&info, dng_all_md);

    get_sensor_crop_info_from_dng(&info, dng_all_md);

    get_lsc_md_from_dng(exif, info.fIFD[info.fMainIndex], dng_all_md);

    
    uint32_t profile_cnt = negative->ProfileCount();
    dng_all_md.hsv_lut_md.twoD_enable = 0;
    dng_all_md.hsv_lut_md.threeD_enable = 0;
    for (uint32_t i = 0; i < profile_cnt; i++)
    {
        const dng_camera_profile& pf= negative->ProfileByIndex(i);
        log_info("profile name:%s\n", pf.Name().Get());
        if (info.fShared->fCameraProfile.fProfileName == pf.Name().Get())
        {
            const dng_tone_curve& curve = pf.ToneCurve();
            size_t sz = curve.fCoord.size();
            log_info("tone curve size %lld:\n", sz);
            for (size_t j = 0; j < sz; j++)
            {
                log_info("%lf, %lf\n", curve.fCoord[j].h, curve.fCoord[j].v);
            }
            if (pf.HasHueSatDeltas())
            {
                dng_all_md.hsv_lut_md.twoD_enable = 1;
                dng_all_md.hsv_lut_md.fHueSatMapEncoding = pf.HueSatMapEncoding();
                dng_all_md.hsv_lut_md.fHueSatDeltas1 = pf.HueSatDeltas1();
                dng_all_md.hsv_lut_md.fHueSatDeltas2 = pf.HueSatDeltas2();
                dng_all_md.hsv_lut_md.fCalibrationIlluminant1 = pf.CalibrationIlluminant1();
                dng_all_md.hsv_lut_md.fCalibrationIlluminant2 = pf.CalibrationIlluminant2();

                if (pf.HasLookTable())
                {
                    dng_all_md.hsv_lut_md.threeD_enable = 1;
                    dng_all_md.hsv_lut_md.fLookTable = pf.LookTable();
                    dng_all_md.hsv_lut_md.fLookTableEncoding = pf.LookTableEncoding();
                }
            }
        }
    }
}

static void read_pnm(const char* file_name, std::vector<data_buffer*> *out)
{
    FILE *input_f = fopen(file_name, "rb");

    if (input_f == nullptr)
    {
        log_error("read pnm fail\n");
        return;
    }

    char p6[4] = {0};
    char width[8] = {0};
    char height[8] = {0};
    char max_val[12] = {0};
    fscanf(input_f, "%s %s %s %s", p6, width, height, max_val);
    if (ftell(input_f) % 2 == 1)
    {
        fseek(input_f, 1L, SEEK_CUR);
    }
    log_info("%s %s %s %s", p6, width, height, max_val);
    if (strcmp(p6, "P6") != 0)
    {
        log_error("pnm must be P6\n");
        fclose(input_f);
        return;
    }

    int32_t pnm_w = atoi(width);
    int32_t pnm_h = atoi(height);
    int32_t max_v = atoi(max_val);

    int32_t max_b = 0;
    for (; max_b <= 16; max_b++)
    {
        if ((1U << max_b) - 1 >= (uint32_t)max_v)
        {
            log_info("pnm bit width %d\n", max_b);
            break;
        }
    }
    if (max_b == 0 || max_b > 16)
    {
        log_error("pnm read bit width fail\n");
        return;
    }
    data_buffer *out0 = new data_buffer(pnm_w, pnm_h, DATA_TYPE_R, BAYER_UNSUPPORT, "raw_in out0");
    data_buffer *out1 = new data_buffer(pnm_w, pnm_h, DATA_TYPE_G, BAYER_UNSUPPORT, "raw_in out1");
    data_buffer *out2 = new data_buffer(pnm_w, pnm_h, DATA_TYPE_B, BAYER_UNSUPPORT, "raw_in out2");
    (*out)[0] = out0;
    (*out)[1] = out1;
    (*out)[2] = out2;
    uint16_t *r_buffer = out0->data_ptr;
    uint16_t *g_buffer = out1->data_ptr;
    uint16_t *b_buffer = out2->data_ptr;
    if (max_b > 8)
    {
        std::unique_ptr<uint16_t[]> tmp_buffer(new uint16_t[pnm_w * pnm_h * 3]);
        uint16_t *tmp_buffer_ptr = tmp_buffer.get();
        size_t read_cnt = fread(tmp_buffer_ptr, sizeof(uint16_t), pnm_w * pnm_h * 3, input_f);
        if (read_cnt != pnm_w * pnm_h * 3)
        {
            log_error("read pnm file bytes cnt error\n");
            fclose(input_f);
            return;
        }
        for (int32_t i = 0; i < pnm_h; i++)
        {
            for (int32_t j = 0; j < pnm_w * 3; j++)
            {
                tmp_buffer_ptr[i * pnm_w * 3 + j] = ((tmp_buffer_ptr[i * pnm_w * 3 + j] & 0x00ff) << 8) | ((tmp_buffer_ptr[i * pnm_w * 3 + j] & 0xff00) >> 8);
            }
        }
        for (int32_t i = 0; i < pnm_h; i++)
        {
            for (int32_t j = 0; j < pnm_w; j++)
            {
                r_buffer[i * pnm_w + j] = tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 0] << (16 - max_b);
                g_buffer[i * pnm_w + j] = tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 1] << (16 - max_b);
                b_buffer[i * pnm_w + j] = tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 2] << (16 - max_b);
            }
        }
    }
    else
    {
        std::unique_ptr<uint8_t[]> tmp_buffer(new uint8_t[pnm_w * pnm_h * 3]);
        uint8_t *tmp_buffer_ptr = tmp_buffer.get();
        size_t read_cnt = fread(tmp_buffer_ptr, sizeof(uint8_t), pnm_w * pnm_h * 3, input_f);
        if (read_cnt != pnm_w * pnm_h * 3)
        {
            log_error("read pnm file bytes cnt error\n");
            fclose(input_f);
            return;
        }
        for (int32_t i = 0; i < pnm_h; i++)
        {
            for (int32_t j = 0; j < pnm_w; j++)
            {
                r_buffer[i * pnm_w + j] = ((uint16_t)tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 0]) << (16 - max_b);
                g_buffer[i * pnm_w + j] = ((uint16_t)tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 1]) << (16 - max_b);
                b_buffer[i * pnm_w + j] = ((uint16_t)tmp_buffer_ptr[i * pnm_w * 3 + j * 3 + 2]) << (16 - max_b);
            }
        }
    }
    fclose(input_f);
}

static void read_raw(const char* file_name, const char* bayer_string, uint32_t img_width, uint32_t img_height, 
    uint32_t bit_depth, bool big_endian, std::vector<data_buffer*> *out)
{
    bayer_type_t raw_bayer;
    if (strcmp(bayer_string, "RGGB") == 0)
    {
        raw_bayer = RGGB;
    }
    else if (strcmp(bayer_string, "GRBG") == 0)
    {
        raw_bayer = GRBG;
    }
    else if (strcmp(bayer_string, "GBRG") == 0)
    {
        raw_bayer = GBRG;
    }
    else if (strcmp(bayer_string, "BGGR") == 0)
    {
        raw_bayer = BGGR;
    }
    else if (strcmp(bayer_string, "RGGIR") == 0)
    {
        raw_bayer = RGGIR;
    }
    else if (strcmp(bayer_string, "GRIRG") == 0)
    {
        raw_bayer = GRIRG;
    }
    else if (strcmp(bayer_string, "BGGIR") == 0)
    {
        raw_bayer = BGGIR;
    }
    else if (strcmp(bayer_string, "GBIRG") == 0)
    {
        raw_bayer = GBIRG;
    }
    else if (strcmp(bayer_string, "IRGGR") == 0)
    {
        raw_bayer = IRGGR;
    }
    else if (strcmp(bayer_string, "GIRRG") == 0)
    {
        raw_bayer = GIRRG;
    }
    else if (strcmp(bayer_string, "IRGGB") == 0)
    {
        raw_bayer = IRGGB;
    }
    else if (strcmp(bayer_string, "GIRBG") == 0)
    {
        raw_bayer = GIRBG;
    }
    data_buffer *out0 = new data_buffer(img_width, img_height, DATA_TYPE_RAW, raw_bayer, "raw_in out0");
    (*out)[0] = out0;

    FILE *input_f = fopen(file_name, "rb");

    if (input_f != nullptr)
    {
        if (bit_depth > 8 && bit_depth <= 16)
        {
            size_t read_cnt = fread(out0->data_ptr, sizeof(uint16_t), img_width * img_height, input_f);
            if (read_cnt != img_width * img_height)
            {
                log_error("read raw file bytes unexpected\n");
            }
            if(!big_endian)
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    out0->data_ptr[s] = out0->data_ptr[s] << (16 - bit_depth);
                }
            }
            else{
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    uint16_t val = ((out0->data_ptr[s] & 0xff00) >> 8) | ((out0->data_ptr[s] & 0x00ff) << 8);
                    out0->data_ptr[s] = val << (16 - bit_depth);
                }
            }
        }
        else if (bit_depth <= 8 && bit_depth > 0)
        {
            uint8_t *mid_pos = reinterpret_cast<uint8_t *>(out0->data_ptr + (img_width * img_height) / 2);
            size_t read_cnt = fread(mid_pos, sizeof(uint8_t), img_width * img_height, input_f);
            if (read_cnt != img_width * img_height)
            {
                log_error("read raw file bytes unexpected\n");
            }
            for (uint32_t s = 0; s < img_height * img_width; s++)
            {
                out0->data_ptr[s] = ((uint16_t)mid_pos[s]) << (16 - bit_depth);
            }
        }
        else
        {
            fclose(input_f);
            log_error("not support %u bits raw\n", bit_depth);
            exit(EXIT_FAILURE);
        }
        fclose(input_f);
    }
    else{
        log_error("open file %s fail\n", file_name);
    }
}

static void read_yuv444(const char* file_name, const char* module_name, uint32_t img_width, uint32_t img_height, 
    uint32_t bit_depth, bool big_endian, std::vector<data_buffer*> *out)
{
    std::string out_name(module_name);
    std::string out0_name = out_name + " out0";
    std::string out1_name = out_name + " out1";
    std::string out2_name = out_name + " out2";

    data_buffer *out0 = new data_buffer(img_width, img_height, DATA_TYPE_Y, BAYER_UNSUPPORT, out0_name.c_str());
    (*out)[0] = out0;
    data_buffer *out1 = new data_buffer(img_width, img_height, DATA_TYPE_U, BAYER_UNSUPPORT, out1_name.c_str());
    (*out)[1] = out1;
    data_buffer *out2 = new data_buffer(img_width, img_height, DATA_TYPE_V, BAYER_UNSUPPORT, out2_name.c_str());
    (*out)[2] = out2;

    FILE *input_f = fopen(file_name, "rb");

    if (input_f != nullptr)
    {
        if (bit_depth > 8 && bit_depth <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_yuv(new uint16_t[img_width * img_height * 3]);
            uint16_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint16_t), img_width * img_height * 3, input_f);
            if (read_cnt != img_width * img_height * 3)
            {
                log_error("read raw file bytes unexpected\n");
            }
            if(!big_endian)
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    out0->data_ptr[s] = buffer_yuv_ptr[s * 3 + 0] << (16 - bit_depth);
                    out1->data_ptr[s] = buffer_yuv_ptr[s * 3 + 1] << (16 - bit_depth);
                    out2->data_ptr[s] = buffer_yuv_ptr[s * 3 + 2] << (16 - bit_depth);
                }
            }
            else{
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    uint16_t val = ((buffer_yuv_ptr[s * 3 + 0] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 3 + 0] & 0x00ff) << 8);
                    out0->data_ptr[s] = val << (16 - bit_depth);
                    val = ((buffer_yuv_ptr[s * 3 + 1] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 3 + 1] & 0x00ff) << 8);
                    out1->data_ptr[s] = val << (16 - bit_depth);
                    val = ((buffer_yuv_ptr[s * 3 + 2] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 3 + 2] & 0x00ff) << 8);
                    out2->data_ptr[s] = val << (16 - bit_depth);
                }
            }
        }
        else if (bit_depth <= 8 && bit_depth > 0)
        {
            std::unique_ptr<uint8_t[]> buffer_yuv(new uint8_t[img_width * img_height * 3]);
            uint8_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint8_t), img_width * img_height * 3, input_f);
            if (read_cnt != img_width * img_height * 3)
            {
                log_error("read raw file bytes unexpected\n");
            }

            for (uint32_t s = 0; s < img_height * img_width; s++)
            {
                out0->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 3 + 0]) << (16 - bit_depth);
                out1->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 3 + 1]) << (16 - bit_depth);
                out2->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 3 + 2]) << (16 - bit_depth);
            }
        }
        else
        {
            fclose(input_f);
            log_error("not support %u bits raw\n", bit_depth);
            exit(EXIT_FAILURE);
        }
        fclose(input_f);
    }
    else{
        log_error("open file %s fail\n", file_name);
        exit(EXIT_FAILURE);
    }
}

static void read_yuv422(const char* file_name, const char* module_name, uint32_t img_width, uint32_t img_height, 
    uint32_t bit_depth, bool big_endian, std::vector<data_buffer*> *out)
{
    //UYVY
    std::string out_name(module_name);
    std::string out0_name = out_name + " out0";
    std::string out1_name = out_name + " out1";
    std::string out2_name = out_name + " out2";

    data_buffer *out0 = new data_buffer(img_width, img_height, DATA_TYPE_Y, BAYER_UNSUPPORT, out0_name.c_str());
    (*out)[0] = out0;
    data_buffer *out1 = new data_buffer(img_width/2, img_height, DATA_TYPE_U, BAYER_UNSUPPORT, out1_name.c_str());
    (*out)[1] = out1;
    data_buffer *out2 = new data_buffer(img_width/2, img_height, DATA_TYPE_V, BAYER_UNSUPPORT, out2_name.c_str());
    (*out)[2] = out2;

    FILE *input_f = fopen(file_name, "rb");

    if (input_f != nullptr)
    {
        if (bit_depth > 8 && bit_depth <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_yuv(new uint16_t[img_width * img_height * 2]);
            uint16_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint16_t), img_width * img_height * 2, input_f);
            if (read_cnt != img_width * img_height * 2)
            {
                log_error("read raw file bytes unexpected\n");
            }
            if(!big_endian)
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    out0->data_ptr[s] = buffer_yuv_ptr[s * 2 + 1] << (16 - bit_depth);
                }
                for (uint32_t s = 0; s < img_height * img_width / 2; s++)
                {
                    out1->data_ptr[s] = buffer_yuv_ptr[s * 4] << (16 - bit_depth);
                    out2->data_ptr[s] = buffer_yuv_ptr[s * 4 + 2] << (16 - bit_depth);
                }
            }
            else
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    uint16_t val = ((buffer_yuv_ptr[s * 2 + 1] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 2 + 1] & 0x00ff) << 8);
                    out0->data_ptr[s] = val << (16 - bit_depth);
                }
                for (uint32_t s = 0; s < img_height * img_width / 2; s++)
                {
                    uint16_t val = ((buffer_yuv_ptr[s * 4] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 4] & 0x00ff) << 8);
                    out1->data_ptr[s] = val << (16 - bit_depth);
                    val = ((buffer_yuv_ptr[s * 4 + 2] & 0xff00) >> 8) | ((buffer_yuv_ptr[s * 4 + 2] & 0x00ff) << 8);
                    out2->data_ptr[s] = val << (16 - bit_depth);
                }

            }
        }
        else if (bit_depth <= 8 && bit_depth > 0)
        {
            std::unique_ptr<uint8_t[]> buffer_yuv(new uint8_t[img_width * img_height * 2]);
            uint8_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint8_t), img_width * img_height * 2, input_f);
            if (read_cnt != img_width * img_height * 2)
            {
                log_error("read raw file bytes unexpected\n");
            }

            for (uint32_t s = 0; s < img_height * img_width; s++)
            {
                out0->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 2 + 1]) << (16 - bit_depth);
            }
            for (uint32_t s = 0; s < img_height * img_width / 2; s++)
            {
                out1->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 4]) << (16 - bit_depth);
                out2->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s * 4 + 2]) << (16 - bit_depth);
            }
        }
        else
        {
            fclose(input_f);
            log_error("not support %u bits raw\n", bit_depth);
            exit(EXIT_FAILURE);
        }
        fclose(input_f);
    }
    else{
        log_error("open file %s fail\n", file_name);
        exit(EXIT_FAILURE);
    }
}

static void read_yuv420(const char* file_name, const char* module_name, uint32_t img_width, uint32_t img_height, 
    uint32_t bit_depth, bool big_endian, std::vector<data_buffer*> *out)
{
    //NV12
    std::string out_name(module_name);
    std::string out0_name = out_name + " out0";
    std::string out1_name = out_name + " out1";
    std::string out2_name = out_name + " out2";

    data_buffer *out0 = new data_buffer(img_width, img_height, DATA_TYPE_Y, BAYER_UNSUPPORT, out0_name.c_str());
    (*out)[0] = out0;
    data_buffer *out1 = new data_buffer(img_width/2, img_height/2, DATA_TYPE_U, BAYER_UNSUPPORT, out1_name.c_str());
    (*out)[1] = out1;
    data_buffer *out2 = new data_buffer(img_width/2, img_height/2, DATA_TYPE_V, BAYER_UNSUPPORT, out2_name.c_str());
    (*out)[2] = out2;

    FILE *input_f = fopen(file_name, "rb");

    if (input_f != nullptr)
    {
        if (bit_depth > 8 && bit_depth <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_yuv(new uint16_t[img_width * img_height * 3 / 2]);
            uint16_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint16_t), img_width * img_height * 3 / 2, input_f);
            if (read_cnt != img_width * img_height * 3 / 2)
            {
                log_error("read raw file bytes unexpected\n");
            }
            if(!big_endian)
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    out0->data_ptr[s] = buffer_yuv_ptr[s] << (16 - bit_depth);
                }
                uint32_t y_oft = img_height * img_width;
                for (uint32_t s = 0; s < img_height * img_width / 4; s++)
                {
                    out1->data_ptr[s] = buffer_yuv_ptr[y_oft + s * 2] << (16 - bit_depth);
                    out2->data_ptr[s] = buffer_yuv_ptr[y_oft + s * 2 + 1] << (16 - bit_depth);
                }
            }
            else
            {
                for (uint32_t s = 0; s < img_height * img_width; s++)
                {
                    uint16_t val = ((buffer_yuv_ptr[s] & 0xff00) >> 8) | ((buffer_yuv_ptr[s] & 0x00ff) << 8);
                    out0->data_ptr[s] = val << (16 - bit_depth);
                }
                uint32_t y_oft = img_height * img_width;
                for (uint32_t s = 0; s < img_height * img_width / 4; s++)
                {
                    uint16_t val = ((buffer_yuv_ptr[y_oft + s * 2] & 0xff00) >> 8) | ((buffer_yuv_ptr[y_oft + s * 2] & 0x00ff) << 8);
                    out1->data_ptr[s] = val << (16 - bit_depth);
                    val = ((buffer_yuv_ptr[y_oft + s * 2 + 1] & 0xff00) >> 8) | ((buffer_yuv_ptr[y_oft + s * 2 + 1] & 0x00ff) << 8);
                    out2->data_ptr[s] = val << (16 - bit_depth);
                }

            }
        }
        else if (bit_depth <= 8 && bit_depth > 0)
        {
            std::unique_ptr<uint8_t[]> buffer_yuv(new uint8_t[img_width * img_height * 3 / 2]);
            uint8_t* buffer_yuv_ptr = buffer_yuv.get();
            size_t read_cnt = fread(buffer_yuv_ptr, sizeof(uint8_t), img_width * img_height * 3 / 2, input_f);
            if (read_cnt != img_width * img_height * 3 / 2)
            {
                log_error("read raw file bytes unexpected\n");
            }

            for (uint32_t s = 0; s < img_height * img_width; s++)
            {
                out0->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[s]) << (16 - bit_depth);
            }
            uint32_t y_oft = img_height * img_width;
            for (uint32_t s = 0; s < img_height * img_width / 4; s++)
            {
                out1->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[y_oft + s * 2]) << (16 - bit_depth);
                out2->data_ptr[s] = ((uint16_t)buffer_yuv_ptr[y_oft + s * 2 + 1]) << (16 - bit_depth);
            }
        }
        else
        {
            fclose(input_f);
            log_error("not support %u bits raw\n", bit_depth);
            exit(EXIT_FAILURE);
        }
        fclose(input_f);
    }
    else{
        log_error("open file %s fail\n", file_name);
        exit(EXIT_FAILURE);
    }
}

void file_read::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start frame %d\n", __FUNCTION__, frame_cnt);
    if (frame_cnt == 0)
    {
        pipeline_manager* cur_pipe_manager = pipeline_manager::get_current_pipe_manager();
        dng_md_t& dng_all_md = cur_pipe_manager->global_ref_out->dng_meta_data;
        if (strcmp(file_type_string, "RAW") == 0 || strcmp(file_type_string, "raw") == 0)
        {
            read_raw(file_name, bayer_string, img_width, img_height, bit_depth, big_endian, &out);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
        }
        else if (strcmp(file_type_string, "DNG") == 0 || strcmp(file_type_string, "dng") == 0)
        {
            data_buffer* out0 = nullptr;
            uint32_t bit_dep = 0;
            readDNG_by_adobe_sdk(file_name, &out0, &bit_dep);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
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
        else if(strcmp(file_type_string, "YUV444") == 0 || strcmp(file_type_string, "yuv444") == 0)//YUVYUVYUV
        {
            read_yuv444(file_name, name, img_width, img_height, bit_depth, big_endian, &out);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
        }
        else if(strcmp(file_type_string, "YUV422") == 0 || strcmp(file_type_string, "yuv422") == 0)//UYVY
        {
            read_yuv422(file_name, name, img_width, img_height, bit_depth, big_endian, &out);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
        }
        else if(strcmp(file_type_string, "YUV420") == 0 || strcmp(file_type_string, "yuv420") == 0)//NV12
        {
            read_yuv420(file_name, name, img_width, img_height, bit_depth, big_endian, &out);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
        }
        else if(strcmp(file_type_string, "PNM") == 0 || strcmp(file_type_string, "pnm") == 0)
        {
            read_pnm(file_name, &out);
            dng_all_md.input_file_name = std::string(file_name);
            stat_out->input_file_name = std::string(file_name);
        }
        else
        {
            log_warning("%s not support yet\n", file_type_string);
        }
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end frame %d\n", __FUNCTION__, frame_cnt);
}

void file_read::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bayer_type",     STRING,     this->bayer_string,      32},
        {"bit_depth",      UINT_32,    &this->bit_depth           },
        {"file_type",      STRING,     this->file_type_string,  32},
        {"file_name",      STRING,     this->file_name,        256},
        {"img_width",      UINT_32,    &this->img_width           },
        {"img_height",     UINT_32,    &this->img_height          },
        {"big_endian",     BOOL_T,     &this->big_endian          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

file_read::~file_read()
{
    log_info("%s deinit start\n", __FUNCTION__);
    log_info("%s deinit end\n", __FUNCTION__);
}