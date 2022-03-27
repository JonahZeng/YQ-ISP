#include "yuv_encode.h"
#include "jpeglib.h"
#include <assert.h>

#include "meta_data.h"

extern dng_md_t g_dng_all_md;

yuv_encode::yuv_encode(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
}

static int32_t yuv_encode_core(const char* out_name, uint32_t y_width, uint32_t y_height, uint8_t* buffer_y, uint8_t* buffer_u, uint8_t* buffer_v)
{
    

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE* outfile = NULL;

    outfile = fopen(out_name, "wb");

    if (outfile == NULL)
    {
        jpeg_destroy_compress(&cinfo);
        log_error("open %s fail\n", out_name);
        return EXIT_FAILURE;
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = y_width;
    cinfo.image_height = y_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&cinfo);
    cinfo.raw_data_in = TRUE;
    cinfo.do_fancy_downsampling = FALSE;

    cinfo.comp_info[0].h_samp_factor = 2;		//for Y
    cinfo.comp_info[0].v_samp_factor = 1;
    cinfo.comp_info[1].h_samp_factor = 1;		//for Cb
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;		//for Cr
    cinfo.comp_info[2].v_samp_factor = 1;

    log_info("block size=%d\n", cinfo.block_size);

    cinfo.comp_info[0].width_in_blocks = y_width / cinfo.block_size;
    cinfo.comp_info[0].height_in_blocks = y_height / cinfo.block_size;
    cinfo.comp_info[1].width_in_blocks = (y_width / 2) / cinfo.block_size;
    cinfo.comp_info[1].height_in_blocks = y_height / cinfo.block_size;
    cinfo.comp_info[2].width_in_blocks = (y_width / 2) / cinfo.block_size;
    cinfo.comp_info[2].height_in_blocks = y_height / cinfo.block_size;

    if (cinfo.comp_info[0].width_in_blocks != 2 * cinfo.comp_info[1].width_in_blocks)
    {
        log_warning("y hor block != 2 * uv hor block\n");
        jpeg_destroy_compress(&cinfo);
        fclose(outfile);
        return EXIT_FAILURE;
    }

    jpeg_set_quality(&cinfo, 100, FALSE);

    JSAMPARRAY yuvbuffer[3];
    JSAMPARRAY y_ptr;
    JSAMPARRAY u_ptr;
    JSAMPARRAY v_ptr;

    int32_t y_row_stride, y_col_stride;
    int32_t u_row_stride, u_col_stride, v_row_stride, v_col_stride;

    y_row_stride = cinfo.comp_info[0].width_in_blocks*cinfo.block_size;
    y_col_stride = cinfo.comp_info[0].height_in_blocks*cinfo.block_size;
    u_row_stride = cinfo.comp_info[1].width_in_blocks*cinfo.block_size;
    u_col_stride = cinfo.comp_info[1].height_in_blocks*cinfo.block_size;
    v_row_stride = cinfo.comp_info[2].width_in_blocks*cinfo.block_size;
    v_col_stride = cinfo.comp_info[2].height_in_blocks*cinfo.block_size;

    log_info("y width=%d, height=%d, u width=%d, height=%d, v width=%d, height=%d\n", y_row_stride, y_col_stride, u_row_stride, u_col_stride, \
        v_row_stride, v_col_stride);

    if (y_row_stride != y_width || y_col_stride != y_height)
    {
        log_warning("final y size != y_width, y_height\n");
        jpeg_destroy_compress(&cinfo);
        fclose(outfile);
        return EXIT_FAILURE;
    }

    y_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*y_col_stride);
    for (int j = 0; j < y_col_stride; j++)
    {
        y_ptr[j] = buffer_y + y_row_stride * j;
    }
    u_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*u_col_stride);
    for (int j = 0; j < u_col_stride; j++)
    {
        u_ptr[j] = buffer_u + u_row_stride * j;
    }
    v_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*v_col_stride);
    for (int j = 0; j < v_col_stride; j++)
    {
        v_ptr[j] = buffer_v + v_row_stride * j;
    }

    yuvbuffer[0] = y_ptr;
    yuvbuffer[1] = u_ptr;
    yuvbuffer[2] = v_ptr;

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        jpeg_write_raw_data(&cinfo, yuvbuffer, cinfo.comp_info[0].v_samp_factor*cinfo.block_size);
        yuvbuffer[0] += cinfo.comp_info[0].v_samp_factor*cinfo.block_size;
        yuvbuffer[1] += cinfo.comp_info[1].v_samp_factor*cinfo.block_size;
        yuvbuffer[2] += cinfo.comp_info[2].v_samp_factor*cinfo.block_size;
    }
    free(y_ptr); free(u_ptr); free(v_ptr);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    return 0;
}

void yuv_encode::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input_y = in[0];
    data_buffer* input_u = in[1];
    data_buffer* input_v = in[2];

    uint32_t full_xsize = input_y->width;
    uint32_t full_ysize = input_y->height;

    uint32_t cb_xsize = input_u->width;
    uint32_t cb_ysize = input_u->height;

    uint32_t cr_xsize = input_v->width;
    uint32_t cr_ysize = input_v->height;

    assert(full_xsize == 2* cb_xsize && full_ysize == cb_ysize);
    assert(cb_xsize % 2 == 0);
    assert(cb_xsize == cr_xsize && cb_ysize == cr_ysize);

    uint8_t* buffer_y = new uint8_t[full_xsize * full_ysize];
    uint8_t* buffer_u = new uint8_t[cb_xsize * cb_ysize];
    uint8_t* buffer_v = new uint8_t[cr_xsize * cr_ysize];

    for (uint32_t sz = 0; sz < full_xsize * full_ysize; sz++)
    {
        buffer_y[sz] = input_y->data_ptr[sz] >> (16 - 8);
    }
    for (uint32_t sz = 0; sz < cb_xsize * cb_ysize; sz++)
    {
        buffer_u[sz] = input_u->data_ptr[sz] >> (16 - 8);
    }
    for (uint32_t sz = 0; sz < cr_xsize * cr_ysize; sz++)
    {
        buffer_v[sz] = input_v->data_ptr[sz] >> (16 - 8);
    }

    if (bypass == 0)
    {
        std::string* input_file_name = &g_dng_all_md.input_file_name;
        std::string output_raw_fn;
        if (use_input_file_name > 0)
        {
            size_t ridx1 = input_file_name->rfind('/');
            size_t ridx2 = input_file_name->rfind('.');
            output_raw_fn = input_file_name->substr(ridx1 + 1, ridx2 - ridx1 - 1);
            output_raw_fn.append(".jpg");
            log_info("target jpg name: %s\n", output_raw_fn.c_str());
        }
        else
        {
            output_raw_fn = std::string(output_jpg_file_name);
        }
        int32_t ret = yuv_encode_core(output_raw_fn.c_str(), full_xsize, full_ysize, buffer_y, buffer_u, buffer_v);
        if (ret != 0)
        {
            log_warning("write jpg fail %d\n", ret);
        }
    }

    delete[] buffer_y;
    delete[] buffer_u;
    delete[] buffer_v;

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void yuv_encode::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",               UINT_32,      &this->bypass},
        {"use_input_file_name",  UINT_32,      &this->use_input_file_name},
        {"output_jpg_file_name", STRING,       this->output_jpg_file_name, 256}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s init run end\n", name);
}

yuv_encode::~yuv_encode()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    log_info("%s module deinit end\n", __FUNCTION__);
}
