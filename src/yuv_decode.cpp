#include "yuv_decode.h"
#include "jpeglib.h"
#include <setjmp.h>
#ifndef _MSC_VER
#include <cstdlib>
#endif

yuv_decode::yuv_decode(uint32_t inpins, uint32_t outpins, const char* inst_name) :hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    do_fancy_upsampling = 1;
}

typedef struct _Tiff_header
{
    char endian_order[2];//"II" or "MM"
    uint16_t tiff_num; //must be 42
    uint32_t first_ifd_offset;
}Tiff_header;

typedef struct _IFD_entry
{
    uint16_t tag;
    uint16_t type;
    uint32_t count;
    uint32_t offset;
}IFD_entry;

typedef struct _IFD
{
    uint16_t IFD_entry_cnt;
    IFD_entry* entry_list;
    uint32_t next_IFD_offset;
}IFD;

static uint16_t convertBigEndian2LittenEndian_u16(uint16_t in)
{
    uint16_t high = (in & 0x00ff) << 8;
    uint16_t low = (in & 0xff00) >> 8;
    return high | low;
}

static uint32_t convertBigEndian2LittenEndian_u32(uint32_t in)
{
    uint32_t a0 = (in & 0x000000ff) << 24;
    uint32_t a1 = (in & 0x0000ff00) << 8;
    uint32_t a2 = (in & 0x00ff0000) >> 8;
    uint32_t a3 = (in & 0xff000000) >> 24;
    return a0 | a1 | a2 | a3;
}

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;  /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

typedef struct _decoded_yuv_t
{
    int32_t orientation;
    int32_t image_width;
    int32_t image_height;
    int32_t buffer_y_width;
    int32_t buffer_y_height;
    int32_t buffer_u_width;
    int32_t buffer_u_height;
    int32_t buffer_v_width;
    int32_t buffer_v_height;
    JSAMPLE* buffer_y_ptr;
    JSAMPLE* buffer_u_ptr;
    JSAMPLE* buffer_v_ptr;
}decoded_yuv_t;

void my_error_exit(j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr)cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

static uint32_t get_orientation(jpeg_saved_marker_ptr marker_ptr)
{
    if (marker_ptr == nullptr)
        return 1;
    while (marker_ptr != nullptr && marker_ptr->marker != JPEG_APP0 + 1)
    {
        marker_ptr = marker_ptr->next;
    }
    if (marker_ptr == nullptr)
        return 1;

    log_info("marker:%d, %d, %d\n", marker_ptr->marker, marker_ptr->original_length, marker_ptr->data_length);

    char exif_code[6] = { '\0' };
    memcpy(exif_code, marker_ptr->data, 6);
    if (strcmp(exif_code, "Exif\0") != 0)
    {
        log_info("warnning: no exif found.\n");
        return 1;
    }
    else
    {
        log_info("exif found !\n");
    }

    Tiff_header* header = (Tiff_header*)(marker_ptr->data + 6); // "Exif\0\0"
    log_info("tiff byte order: %c%c || MM=big-endian, II=litter-endian\n", header->endian_order[0], header->endian_order[1]);
    int endian = 0;//0 = litter-endian, 1 = big-endian
    if (header->endian_order[0] == 'M' && header->endian_order[1] == 'M')
        endian = 1;
    if (endian == 0)
    {
        log_info("tiff magic number:%d\n", header->tiff_num);
        log_info("APP1 firset IFD offset = %d\n", header->first_ifd_offset);
    }
    else
    {
        header->tiff_num = convertBigEndian2LittenEndian_u16(header->tiff_num);
        log_info("tiff magic number:%d\n", header->tiff_num);
        header->first_ifd_offset = convertBigEndian2LittenEndian_u32(header->first_ifd_offset);
        log_info("APP1 firset IFD offset = %d\n", header->first_ifd_offset);
    }

    uint32_t ifd_offset = header->first_ifd_offset;
    if (ifd_offset < 8)
    {
        log_info("no ifd in APP1\n");
        return 1;
    }
    int find_orientation_tag = FALSE;
    uint16_t orientation = 1;

    while (ifd_offset >= 8)
    {
        IFD ifd_0;
        ifd_0.IFD_entry_cnt = *(uint16_t*)(marker_ptr->data + 6 + ifd_offset);
        if (endian == 1)
        {
            ifd_0.IFD_entry_cnt = convertBigEndian2LittenEndian_u16(ifd_0.IFD_entry_cnt);
        }
        log_info("ifd entry cnt = %d\n", ifd_0.IFD_entry_cnt);

        ifd_0.entry_list = (IFD_entry*)(marker_ptr->data + 6 + ifd_offset + 2);
        int i = 0;
        while (i < ifd_0.IFD_entry_cnt)
        {
            IFD_entry* entry = ifd_0.entry_list + i;
            if (endian == 1)
            {
                entry->tag = convertBigEndian2LittenEndian_u16(entry->tag);
            }
            log_info("ifd  tag:%d\n", entry->tag);
            if (entry->tag == 274)
            {
                find_orientation_tag = TRUE;
                orientation = (uint16_t)entry->offset;
                if (endian == 1)
                {
                    orientation = convertBigEndian2LittenEndian_u16(orientation);
                    log_info("orientation: %d\n", orientation);
                }
            }
            i++;
        }

        ifd_0.next_IFD_offset = *(uint32_t*)(marker_ptr->data + 6 + ifd_offset + ifd_0.IFD_entry_cnt * 12 + 2);
        if (endian == 1)
        {
            ifd_0.next_IFD_offset = convertBigEndian2LittenEndian_u32(ifd_0.next_IFD_offset);
        }
        ifd_offset = ifd_0.next_IFD_offset;
        log_info("next ifd (offset relative to tiff header):%d\n", ifd_offset);
    }


    if (find_orientation_tag == FALSE)
    {
        log_info("no orientation tag found !\n");
        return 1;
    }
    else
    {
        //IFD_entry* orientation_entry = exif_IFD.entry_list + j;
        //if (endian == 1)
        //    orientation_entry->offset = convertBigEndian2LittenEndian_u32(orientation_entry->offset);
        //return orientation_entry->offset;
        return orientation;
    }
}



static int32_t read_raw_data_from_JPEG_file(const char* filename, uint32_t do_fancy_upsample, decoded_yuv_t* out_yuv)
{
    struct jpeg_decompress_struct cinfo;

    struct my_error_mgr jerr;

    FILE * infile = nullptr;      /* source file */

    JSAMPARRAY yuvbuffer[3];
    JSAMPARRAY y_ptr;
    JSAMPARRAY u_ptr;
    JSAMPARRAY v_ptr;
    int y_row_stride, y_col_stride;     /* physical row width in output buffer */
    uint32_t y_row_blocks = 0, y_col_blocks = 0;
    int u_row_stride, u_col_stride, v_row_stride, v_col_stride;

    log_info("run %s\n", __FUNCTION__);

    infile = fopen(filename, "rb");

    if (infile == nullptr)
    {
        log_error("can't open %s\n", filename);
        return -1;
    }


    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {

        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return -1;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, infile);

    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);

    (void)jpeg_read_header(&cinfo, TRUE);
    //注意，在读取了头信息以后，这个时候获取了默认解压参数，我们需要做3个修改
    //1, 修改输出数据的色域，默认是rgb，这里指定为YCbCr
    cinfo.out_color_space = cinfo.jpeg_color_space;
    //设置输出raw data，也就是yuv数据
    cinfo.raw_data_out = TRUE;
    //如果想得到yuv444，这个值设置为TRUE。如果想得到yuv422/420，则设置为FALSE
    if(do_fancy_upsample > 0)
    {
        cinfo.do_fancy_upsampling = TRUE;
    }
    else{
        cinfo.do_fancy_upsampling = FALSE;
    }
    log_info("    JCS_UNKNOWN=0,		 error/unspecified\n \
    JCS_GRAYSCALE=1,		monochrome\n \
    JCS_RGB=2,		red/green/blue, standard RGB (sRGB)\n \
    JCS_YCbCr=3,		 Y/Cb/Cr (also known as YUV), standard YCC\n \
    JCS_CMYK=4,		C/M/Y/K\n \
    JCS_YCCK=5,		Y/Cb/Cr/K\n \
    JCS_BG_RGB=6,		big gamut red/green/blue, bg-sRGB\n \
    JCS_BG_YCC=7		big gamut Y/Cb/Cr, bg-sYCC \n");
    log_info("jpeg file color space: %d, out file color space:%d\n", cinfo.jpeg_color_space, cinfo.out_color_space);

    int orientation = get_orientation(cinfo.marker_list);
    (void)jpeg_start_decompress(&cinfo);
    //调用jpeg_start_decompress后, cinfo会获取到头yuv比例，比如yuv420的话，cinfo.comp_info[0].h_samp_factor/v_samp_factor = 2， uv的 h_samp_factor/v_samp_factor = 1
    if (cinfo.comp_info != nullptr) {
        log_info("compenent[0] width_in_block=%d, y horizontal factor=%d\n", cinfo.comp_info[0].width_in_blocks, cinfo.comp_info[0].h_samp_factor);
        log_info("compenent[0] height_in_blocks=%d, y vertical factor=%d\n", cinfo.comp_info[0].height_in_blocks, cinfo.comp_info[0].v_samp_factor);
        log_info("compenent[1] width_in_block=%d, u horizontal factor=%d\n", cinfo.comp_info[1].width_in_blocks, cinfo.comp_info[1].h_samp_factor);
        log_info("compenent[1] height_in_blocks=%d, u vertical factor=%d\n", cinfo.comp_info[1].height_in_blocks, cinfo.comp_info[1].v_samp_factor);
        log_info("compenent[2] width_in_block=%d, v horizontal factor=%d\n", cinfo.comp_info[2].width_in_blocks, cinfo.comp_info[2].h_samp_factor);
        log_info("compenent[2] height_in_blocks=%d, v vertical factor=%d\n", cinfo.comp_info[2].height_in_blocks, cinfo.comp_info[2].v_samp_factor);
    }
    log_info("block size=%d\n", cinfo.block_size);
    //这里打印的是DCT block size，通常都是8

    log_info("image output width:%d, height:%d, color components:%d, out colorspace:%d\n", cinfo.output_width, cinfo.output_height, cinfo.out_color_components, cinfo.out_color_space);


    y_row_blocks = cinfo.comp_info[0].width_in_blocks;//获取 y 分量的行宽
    y_col_blocks = cinfo.comp_info[0].height_in_blocks;//获取 y 分量的高度

    while (y_row_blocks < cinfo.comp_info[1].width_in_blocks*cinfo.comp_info[0].h_samp_factor / cinfo.comp_info[1].h_samp_factor \
        || y_row_blocks < cinfo.comp_info[2].width_in_blocks*cinfo.comp_info[0].h_samp_factor / cinfo.comp_info[2].h_samp_factor)
    {
        y_row_blocks++;
    }

    while (y_col_blocks < cinfo.comp_info[1].height_in_blocks*cinfo.comp_info[0].v_samp_factor / cinfo.comp_info[1].v_samp_factor \
        || y_col_blocks < cinfo.comp_info[2].height_in_blocks*cinfo.comp_info[0].v_samp_factor / cinfo.comp_info[2].v_samp_factor)
    {
        y_col_blocks++;
    }
    //计算真正的y row/col,根据uv的DCT block计算
    y_row_stride = cinfo.block_size * y_row_blocks;
    y_col_stride = cinfo.block_size * y_col_blocks;
    u_row_stride = cinfo.comp_info[1].width_in_blocks*cinfo.block_size;
    u_col_stride = cinfo.comp_info[1].height_in_blocks*cinfo.block_size;
    v_row_stride = cinfo.comp_info[2].width_in_blocks*cinfo.block_size;
    v_col_stride = cinfo.comp_info[2].height_in_blocks*cinfo.block_size;
    if (cinfo.do_fancy_upsampling == TRUE)
    {
        u_row_stride = y_row_stride;
        u_col_stride = y_col_stride;
        v_row_stride = y_row_stride;
        v_col_stride = y_col_stride;
    }

    log_info("(output buffer)padded compnent[0]_width=%d, compnent[0]_height=%d\n", y_row_stride, y_col_stride);
    log_info("(output buffer)padded compnent[1]_width=%d, compnent[1]_height=%d\n", u_row_stride, u_col_stride);
    log_info("(output buffer)padded compnent[2]_width=%d, compnent[2]_height=%d\n", v_row_stride, v_col_stride);
    JSAMPLE* buffer_y = (JSAMPLE*)malloc(y_row_stride * y_col_stride);
    JSAMPLE* buffer_u;
    JSAMPLE* buffer_v;

    buffer_u = (JSAMPLE*)malloc(u_row_stride * u_col_stride);
    buffer_v = (JSAMPLE*)malloc(v_row_stride * v_col_stride);

    log_info("buffer address = %p %p % p\n", buffer_y, buffer_u, buffer_v);
    //分配所有的yuv缓存空间
    if (buffer_y == nullptr || buffer_u == nullptr || buffer_v == nullptr)
    {
        log_info("buffer==null\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return -1;
    }
    //把y的指针指向头部
    y_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*y_col_stride);
    if (y_ptr == nullptr)
    {
        log_info("y_ptr==null\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        free(buffer_y); free(buffer_u); free(buffer_v);
        buffer_y = nullptr; buffer_u = nullptr; buffer_v = nullptr;
        return -1;
    }
    for (int j = 0; j < y_col_stride; j++)
    {
        y_ptr[j] = buffer_y + y_row_stride * j;
    }
    //u的指针指向y空间的后面

    u_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*u_col_stride);
    
    if (u_ptr == nullptr)
    {
        log_info("u_ptr==null\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        free(buffer_y); free(buffer_u); free(buffer_v);
        buffer_y = nullptr; buffer_u = nullptr; buffer_v = nullptr;
        free(y_ptr);
        return -1;
    }

    for (int j = 0; j < u_col_stride; j++)
    {
        u_ptr[j] = buffer_u + u_row_stride * j;
    }

    v_ptr = (JSAMPROW*)malloc(sizeof(JSAMPROW*)*v_col_stride);
    
    if (v_ptr == nullptr)
    {
        log_info("v_ptr==null\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        free(buffer_y); free(buffer_u); free(buffer_v);
        buffer_y = nullptr; buffer_u = nullptr; buffer_v = nullptr;
        free(y_ptr);
        free(u_ptr);
        return -1;
    }

    for (int j = 0; j < v_col_stride; j++)
    {
        v_ptr[j] = buffer_v + v_row_stride * j;
    }

    yuvbuffer[0] = y_ptr;
    yuvbuffer[1] = u_ptr;
    yuvbuffer[2] = v_ptr;
    if (cinfo.do_fancy_upsampling == TRUE)
    {
        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_raw_data(&cinfo, yuvbuffer, cinfo.block_size);
            yuvbuffer[0] += cinfo.block_size;
            yuvbuffer[1] += cinfo.block_size;
            yuvbuffer[2] += cinfo.block_size;
            //每次读取DCT*max_v_samp_factor行以后，手动把指针指向的缓存区往后移动
        }
    }
    else 
    {
        while (cinfo.output_scanline < cinfo.output_height) 
        {
            jpeg_read_raw_data(&cinfo, yuvbuffer, cinfo.comp_info[0].v_samp_factor*cinfo.block_size);
            yuvbuffer[0] += cinfo.comp_info[0].v_samp_factor*cinfo.block_size;
            yuvbuffer[1] += cinfo.comp_info[1].v_samp_factor*cinfo.block_size;
            yuvbuffer[2] += cinfo.comp_info[2].v_samp_factor*cinfo.block_size;
            //每次读取DCT*max_v_samp_factor行以后，手动把指针指向的缓存区往后移动
        }
    }

    free(y_ptr); free(u_ptr); free(v_ptr);
    out_yuv->orientation = orientation;
    out_yuv->buffer_y_ptr = buffer_y;
    out_yuv->buffer_u_ptr = buffer_u;
    out_yuv->buffer_v_ptr = buffer_v;
    out_yuv->image_width = cinfo.image_width;
    out_yuv->image_height = cinfo.image_height;
    out_yuv->buffer_y_width = cinfo.block_size * y_row_blocks;
    out_yuv->buffer_y_height = cinfo.block_size * y_col_blocks;
    out_yuv->buffer_u_width = cinfo.comp_info[1].width_in_blocks* cinfo.block_size;
    out_yuv->buffer_u_height = cinfo.comp_info[1].height_in_blocks* cinfo.block_size;
    out_yuv->buffer_v_width = cinfo.comp_info[2].width_in_blocks* cinfo.block_size;
    out_yuv->buffer_v_height = cinfo.comp_info[2].height_in_blocks* cinfo.block_size;
    if (cinfo.do_fancy_upsampling == TRUE)
    {
        out_yuv->buffer_u_width = out_yuv->buffer_y_width;
        out_yuv->buffer_u_height = out_yuv->buffer_y_height;
        out_yuv->buffer_v_width = out_yuv->buffer_y_width;
        out_yuv->buffer_v_height = out_yuv->buffer_y_height;
    }
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);

    return 0;
}

void yuv_decode::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);

    decoded_yuv_t out_yuv;
    int32_t ret = -1;

    if (strlen(jpg_file_name) > 0)
    {
        ret = read_raw_data_from_JPEG_file(jpg_file_name, do_fancy_upsampling, &out_yuv);
    }
    if (ret == 0)
    {
        uint32_t full_xsize = out_yuv.buffer_y_width;
        uint32_t full_ysize = out_yuv.buffer_y_height;

        uint32_t cb_xsize = out_yuv.buffer_u_width;
        uint32_t cb_ysize = out_yuv.buffer_u_height;

        uint32_t cr_xsize = out_yuv.buffer_v_width;
        uint32_t cr_ysize = out_yuv.buffer_v_height;

        data_buffer* out_0 = new data_buffer(full_xsize, full_ysize, DATA_TYPE_Y, BAYER_UNSUPPORT, "yuv_dencode_out0");
        data_buffer* out_1 = new data_buffer(cb_xsize, cb_ysize, DATA_TYPE_U, BAYER_UNSUPPORT, "yuv_dencode_out1");
        data_buffer* out_2 = new data_buffer(cr_xsize, cr_ysize, DATA_TYPE_V, BAYER_UNSUPPORT, "yuv_dencode_out2");
        out[0] = out_0;
        out[1] = out_1;
        out[2] = out_2;

        if (bypass == 0)
        {
            for (uint32_t sz = 0; sz < full_xsize * full_ysize; sz++)
            {
                out_0->data_ptr[sz] = out_yuv.buffer_y_ptr[sz] << (16 - 8);
            }
            for (uint32_t sz = 0; sz < cb_xsize * cb_ysize; sz++)
            {
                out_1->data_ptr[sz] = out_yuv.buffer_u_ptr[sz] << (16 - 8);
            }
            for (uint32_t sz = 0; sz < cr_xsize * cr_ysize; sz++)
            {
                out_2->data_ptr[sz] = out_yuv.buffer_v_ptr[sz] << (16 - 8);
            }
        }

        free(out_yuv.buffer_y_ptr); free(out_yuv.buffer_u_ptr); free(out_yuv.buffer_v_ptr);
        out_yuv.buffer_y_ptr = nullptr; out_yuv.buffer_u_ptr = nullptr; out_yuv.buffer_v_ptr = nullptr;
    }

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void yuv_decode::hw_init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",        UINT_32,      &this->bypass},
        {"jpg_file_name", STRING,       this->jpg_file_name, 256},
        {"do_fancy_upsampling", UINT_32, &this->do_fancy_upsampling}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }

    hw_base::hw_init();
    log_info("%s init run end\n", name);
}

yuv_decode::~yuv_decode()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    log_info("%s module deinit end\n", __FUNCTION__);
}
