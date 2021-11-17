#include "fe_firmware.h"
#include <assert.h>
#include <math.h>
//#include "opencv2/opencv.hpp"

#define MAX2(a, b) ((a)>(b))?(a):(b)
#define MIN2(a, b) ((a)>(b))?(b):(a)
#define CLIP3(in, min, max) (in<min)?min:((in>max)?max:in)

static uint32_t interp_1d(uint32_t input, uint32_t* in_x, uint32_t* in_y, uint32_t len)
{
    if (input <= in_x[0])
    {
        return in_y[0];
    }
    else if (input > in_x[len - 1])
    {
        return in_y[len - 1];
    }
    for (uint32_t i = 0; i < len - 1; i++)
    {
        if (input > in_x[i] && input <= in_x[i + 1])
        {
            uint32_t delta = (input - in_x[i]) * 256 / (in_x[i + 1] - in_x[i]);
            return in_y[i] + (((in_y[i + 1] - in_y[i])*delta) >> 8);
        }
    }
    return in_y[0];
}

static void cac_rgb_hw_core(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* out_r, uint16_t* out_g, uint16_t* out_b,
    uint32_t xsize, uint32_t ysize, const cac_rgb_reg_t* cac_rgb_reg)
{
    const uint16_t KERNEL_SIZE = 15;
    uint32_t ori_width = xsize;
    uint32_t ori_height = ysize;
    uint32_t ext_width = xsize + KERNEL_SIZE - 1;
    uint32_t ext_height = ysize + KERNEL_SIZE - 1;

    uint16_t* r_ext = new uint16_t[ext_width*ext_height];
    uint16_t* g_ext = new uint16_t[ext_width*ext_height];
    uint16_t* b_ext = new uint16_t[ext_width*ext_height];
    uint16_t* luma_ext = new uint16_t[ext_width*ext_height];

    for (uint32_t row = KERNEL_SIZE / 2; row < ext_height - KERNEL_SIZE / 2; row++)
    {
        for (uint32_t col = KERNEL_SIZE / 2; col < ext_width - KERNEL_SIZE / 2; col++)
        {
            r_ext[row*ext_width + col] = r[(row - KERNEL_SIZE / 2)*ori_width + (col - KERNEL_SIZE / 2)];
            g_ext[row*ext_width + col] = g[(row - KERNEL_SIZE / 2)*ori_width + (col - KERNEL_SIZE / 2)];
            b_ext[row*ext_width + col] = b[(row - KERNEL_SIZE / 2)*ori_width + (col - KERNEL_SIZE / 2)];
        }
    }
    for (uint32_t row = 0; row < KERNEL_SIZE / 2; row++)
    {
        for (uint32_t col = KERNEL_SIZE / 2; col < ext_width - KERNEL_SIZE / 2; col++)
        {
            r_ext[row*ext_width + col] = r[(KERNEL_SIZE / 2 - row)*ori_width + (col - KERNEL_SIZE / 2)];
            g_ext[row*ext_width + col] = g[(KERNEL_SIZE / 2 - row)*ori_width + (col - KERNEL_SIZE / 2)];
            b_ext[row*ext_width + col] = b[(KERNEL_SIZE / 2 - row)*ori_width + (col - KERNEL_SIZE / 2)];
        }
    }
    for (uint32_t row = ori_height + KERNEL_SIZE / 2; row < ext_height; row++)
    {
        for (uint32_t col = KERNEL_SIZE / 2; col < ext_width - KERNEL_SIZE / 2; col++)
        {
            r_ext[row*ext_width + col] = r[(2 * ori_height + KERNEL_SIZE / 2 - row - 2)*ori_width + (col - KERNEL_SIZE / 2)];
            g_ext[row*ext_width + col] = g[(2 * ori_height + KERNEL_SIZE / 2 - row - 2)*ori_width + (col - KERNEL_SIZE / 2)];
            b_ext[row*ext_width + col] = b[(2 * ori_height + KERNEL_SIZE / 2 - row - 2)*ori_width + (col - KERNEL_SIZE / 2)];
        }
    }
    for (uint32_t row = 0; row < ext_height; row++)
    {
        for (uint32_t col = 0; col < KERNEL_SIZE / 2; col++)
        {
            r_ext[row*ext_width + col] = r_ext[row*ext_width + (KERNEL_SIZE / 2) - col];
            g_ext[row*ext_width + col] = g_ext[row*ext_width + (KERNEL_SIZE / 2) - col];
            b_ext[row*ext_width + col] = b_ext[row*ext_width + (KERNEL_SIZE / 2) - col];
        }
    }

    for (uint32_t row = 0; row < ext_height; row++)
    {
        for (uint32_t col = KERNEL_SIZE / 2 + ori_width; col < ext_width; col++)
        {
            r_ext[row*ext_width + col] = r_ext[row*ext_width + 2 * ori_width + (KERNEL_SIZE / 2) * 2 - col - 2];
            g_ext[row*ext_width + col] = g_ext[row*ext_width + 2 * ori_width + (KERNEL_SIZE / 2) * 2 - col - 2];
            b_ext[row*ext_width + col] = b_ext[row*ext_width + 2 * ori_width + (KERNEL_SIZE / 2) * 2 - col - 2];
        }
    }

    for (uint32_t row = 0; row < ext_height; row++)
    {
        for (uint32_t col = 0; col < ext_width; col++)
        {
            luma_ext[row*ext_width + col] = uint16_t(0.299 * r_ext[row*ext_width + col] + 0.587*g_ext[row*ext_width + col]
                + 0.114*b_ext[row*ext_width + col]);
        }
    }

    uint16_t* y_gradient = new uint16_t[ori_height*ori_width];
    //cv::Mat out_rgb(ori_height, ori_width, CV_8UC3);

    bool flag = false;

    for (uint32_t row = 0; row < ori_height; row++)
    {
        for (uint32_t col = 0; col < ori_width; col++)
        {
            if (col == 630 && row == 124)
            {
                flag = true;
            }
            else {
                flag = false;
            }
            uint32_t row_ = KERNEL_SIZE / 2 + row;
            uint32_t col_ = KERNEL_SIZE / 2 + col;
            uint32_t h_grad_sum = 0;
            for (int16_t i = 0 - KERNEL_SIZE / 2; i <= KERNEL_SIZE / 2; i++)
            {
                for (int16_t j = 0 - KERNEL_SIZE / 2; j <= KERNEL_SIZE / 2 - 1; j++)
                {
                    h_grad_sum += abs(luma_ext[(row_ + i)*ext_width + col_ + j + 1] - luma_ext[(row_ + i)*ext_width + col_ + j]);
                }
            }

            uint32_t v_grad_sum = 0;
            for (int16_t i = 0 - KERNEL_SIZE / 2; i <= KERNEL_SIZE / 2 - 1; i++)
            {
                for (int16_t j = 0 - KERNEL_SIZE / 2; j <= KERNEL_SIZE / 2; j++)
                {
                    v_grad_sum += abs(luma_ext[(row_ + i + 1)*ext_width + col_ + j] - luma_ext[(row_ + i)*ext_width + col_ + j]);
                }
            }

            uint32_t d1_grad_sum = 0;
            for (int16_t i = 0 - KERNEL_SIZE / 2; i <= KERNEL_SIZE / 2 - 1; i++)
            {
                for (int16_t j = 0 - KERNEL_SIZE / 2; j <= KERNEL_SIZE / 2 - 1; j++)
                {
                    d1_grad_sum += abs(luma_ext[(row_ + i + 1)*ext_width + col_ + j + 1] - luma_ext[(row_ + i)*ext_width + col_ + j]);
                }
            }

            uint32_t d2_grad_sum = 0;
            for (int16_t i = 0 - KERNEL_SIZE / 2; i <= KERNEL_SIZE / 2 - 1; i++)
            {
                for (int16_t j = 0 - KERNEL_SIZE / 2 + 1; j <= KERNEL_SIZE / 2; j++)
                {
                    d2_grad_sum += abs(luma_ext[(row_ + i + 1)*ext_width + col_ + j - 1] - luma_ext[(row_ + i)*ext_width + col_ + j]);
                }
            }

            uint32_t d1_grad_avg = d1_grad_sum / ((KERNEL_SIZE - 1)*(KERNEL_SIZE - 1));
            uint32_t d2_grad_avg = d2_grad_sum / ((KERNEL_SIZE - 1)*(KERNEL_SIZE - 1));
            uint16_t grad = MAX2(h_grad_sum / (KERNEL_SIZE*(KERNEL_SIZE - 1)), v_grad_sum / (KERNEL_SIZE*(KERNEL_SIZE - 1)));
            grad = MAX2(grad, d1_grad_avg);
            grad = MAX2(grad, d2_grad_avg);
            grad = MIN2(grad, 255);

            uint32_t luma_array[5][5] = { 0 };
            uint32_t high_luma_cnt = 0;
            uint32_t high_luma_thr = 240;
            uint32_t luma_max = 0;
            uint32_t luma_min = 255;

            for (int16_t y = 0 - KERNEL_SIZE / 2; y < KERNEL_SIZE / 2; y++)
            {
                for (int16_t x = 0 - KERNEL_SIZE / 2; x < KERNEL_SIZE / 2; x++)
                {
                    uint32_t luma_idx = (x + KERNEL_SIZE / 2) / 3;
                    uint32_t luma_idy = (y + KERNEL_SIZE / 2) / 3;
                    luma_array[luma_idy][luma_idx] += luma_ext[(row_ + y)*ext_width + (col_ + x)];
                }
            }
            for (int16_t y = 0; y < 5; y++)
            {
                for (int16_t x = 0; x < 5; x++)
                {
                    luma_array[y][x] = luma_array[y][x] / 9;
                    if (luma_array[y][x] > high_luma_thr)
                        high_luma_cnt++;
                    luma_max = MAX2(luma_max, luma_array[y][x]);
                    luma_min = MIN2(luma_max, luma_array[y][x]);
                }
            }

            uint32_t high_luma_cnt_thr[4] = { 0, 3, 12, 25 };
            uint32_t high_luma_cnt_weight[4] = { 0, 192, 224, 256 };
            uint32_t w1 = interp_1d(high_luma_cnt, high_luma_cnt_thr, high_luma_cnt_weight, 4);

            uint32_t luma_contrast_thr[4] = { 0, 100, 200, 256 };
            uint32_t luma_contrast_weight[4] = { 0, 10, 226, 256 };
            uint32_t w2 = interp_1d(luma_max - luma_min, luma_contrast_thr, luma_contrast_weight, 4);

            y_gradient[row*ori_width + col] = grad * w1*w2 / (256 * 256);
            uint32_t r_sum = 0, g_sum = 0, b_sum = 0;
            uint32_t r_avg = 0, g_avg = 0, b_avg = 0;

            for (int16_t y = -2; y <= 2; y++)
            {
                for (int16_t x = -2; x <= 2; x++)
                {
                    r_sum += r_ext[(row_ + y)*ext_width + col_ + x];
                    g_sum += g_ext[(row_ + y)*ext_width + col_ + x];
                    b_sum += b_ext[(row_ + y)*ext_width + col_ + x];
                }
            }
            r_avg = r_sum / 25;
            g_avg = g_sum / 25;
            b_avg = b_sum / 25;
            uint32_t luma_cur = luma_ext[row_*ext_width + col_];
            uint32_t r_cur = r_ext[row_*ext_width + col_];
            uint32_t g_cur = g_ext[row_*ext_width + col_];
            uint32_t b_cur = b_ext[row_*ext_width + col_];

            //int32_t cb = -0.169 * int32_t(r_cur) - 0.331*int32_t(g_cur) + 0.5*int32_t(b_cur);
            //int32_t cr = 0.5 * int32_t(r_cur) - 0.419*int32_t(g_cur) - 0.081*int32_t(b_cur);

            int32_t cb = (-173 * int32_t(r_cur) - 339*int32_t(g_cur) + 512*int32_t(b_cur) + 512)>>10;
            int32_t cr = (512 * int32_t(r_cur) - 429*int32_t(g_cur) - 83*int32_t(b_cur) + 512)>>10;
            int32_t sat = int32_t(sqrt(cr * cr + cb * cb));
            double theta = double(cr) / cb;

            if (sat > 20 && sat<80 && theta>0.1 && theta < 0.6 && luma_cur < 210
                && cb>0 && cr>0 && b_avg > (g_avg + 15))
            {

            }
            else
            {
                y_gradient[row*ori_width + col] = 0;
            }
            if (flag)
            {
                log_info("y grad = %d\n", y_gradient[row*ori_width + col]);
            }
            uint32_t prob_thr[4] = { 3, 8, 18, 30 };
            uint32_t prob_weight[4] = { 0, 192, 224,256 };
            y_gradient[row*ori_width + col] = interp_1d(y_gradient[row*ori_width + col], prob_thr, prob_weight, 4);
            if (flag)
            {
                log_info("y grad = %d\n", y_gradient[row*ori_width + col]);
            }

            int32_t new_cr = cr * (256 - y_gradient[row*ori_width + col]) / 256;
            int32_t new_cb = cb * (256 - y_gradient[row*ori_width + col]) / 256;

            int32_t new_r = (1024 * (int32_t)luma_cur - 1 * new_cb + 1436 * new_cr + 512) >> 10;
            int32_t new_g = (1024 * (int32_t)luma_cur - 353 * new_cb - 731 * new_cr + 512) >> 10;
            int32_t new_b = (1024 * (int32_t)luma_cur + 1815 * new_cb - 1 * new_cr + 512) >> 10;
            new_r = CLIP3(new_r, 0, 255);
            new_g = CLIP3(new_g, 0, 255);
            new_b = CLIP3(new_b, 0, 255);

            if (flag)
            {
                log_info("input rgb= %d, %d, %d, output rgb= %d, %d, %d\n", r_cur, g_cur, b_cur, new_r, new_g, new_b);
                log_info("input yuv= %d, %d, %d, output yuv= %d, %d, %d\n", luma_cur, cb, cr, luma_cur, new_cb, new_cr);
            }

            out_r[row*ori_width + col] = new_r;
            out_g[row*ori_width + col] = new_g;
            out_b[row*ori_width + col] = new_b;
            //out_rgb.data[row*ori_width * 3 + col * 3] = new_b;
            //out_rgb.data[row*ori_width * 3 + col * 3 + 1] = new_g;
            //out_rgb.data[row*ori_width * 3 + col * 3 + 2] = new_r;
        }
    }
    //cv::imwrite("./new_rgb.jpg", out_rgb);

    delete[] r_ext;
    delete[] g_ext;
    delete[] b_ext;
    delete[] luma_ext;
    delete[] y_gradient;
}


cac_rgb::cac_rgb(uint32_t inpins, uint32_t outpins, const char* inst_name):hw_base(inpins, outpins, inst_name)
{
    bypass = 0;
    cac_rgb_reg = new cac_rgb_reg_t;
}

void cac_rgb::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    log_info("%s run start\n", __FUNCTION__);
    data_buffer* input0 = in[0];
    data_buffer* input1 = in[1];
    data_buffer* input2 = in[2];

    if (in.size() > 3)
    {
        fe_module_reg_t* fe_reg = (fe_module_reg_t*)(in[3]->data_ptr);
        memcpy(cac_rgb_reg, &fe_reg->cac_rgb_reg, sizeof(cac_rgb_reg_t));
    }

    cac_rgb_reg->bypass = bypass;
    if (xmlConfigValid)
    {
    }

    checkparameters(cac_rgb_reg);

    uint32_t xsize = input0->width;
    uint32_t ysize = input0->height;

    assert(input0->width == input1->width && input0->width == input2->width);
    assert(input0->height == input1->height && input0->height == input2->height);

    data_buffer* output0 = new data_buffer(xsize, ysize, input0->data_type, input0->bayer_pattern, "cac_rgb_out0");
    out[0] = output0;
    data_buffer* output1 = new data_buffer(xsize, ysize, input1->data_type, input1->bayer_pattern, "cac_rgb_out1");
    out[1] = output1;
    data_buffer* output2 = new data_buffer(xsize, ysize, input2->data_type, input2->bayer_pattern, "cac_rgb_out2");
    out[2] = output2;

    uint16_t* out0_ptr = output0->data_ptr;
    uint16_t* out1_ptr = output1->data_ptr;
    uint16_t* out2_ptr = output2->data_ptr;

    uint16_t* tmp0 = new uint16_t[xsize*ysize];
    uint16_t* tmp1 = new uint16_t[xsize*ysize];
    uint16_t* tmp2 = new uint16_t[xsize*ysize];

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        tmp0[sz] = input0->data_ptr[sz] >> (16 - 8);
        out0_ptr[sz] = tmp0[sz];

        tmp1[sz] = input1->data_ptr[sz] >> (16 - 8);
        out1_ptr[sz] = tmp1[sz];

        tmp2[sz] = input2->data_ptr[sz] >> (16 - 8);
        out2_ptr[sz] = tmp2[sz];
    }

    if (cac_rgb_reg->bypass == 0)
    {
        cac_rgb_hw_core(tmp0, tmp1, tmp2, out0_ptr, out1_ptr, out2_ptr, xsize, ysize, cac_rgb_reg);
    }

    for (uint32_t sz = 0; sz < xsize*ysize; sz++)
    {
        out0_ptr[sz] = out0_ptr[sz] << (16 - 8);
        out1_ptr[sz] = out1_ptr[sz] << (16 - 8);
        out2_ptr[sz] = out2_ptr[sz] << (16 - 8);
    }

    delete[] tmp0;
    delete[] tmp1;
    delete[] tmp2;

    hw_base::hw_run(stat_out, frame_cnt);
    log_info("%s run end\n", __FUNCTION__);
}

void cac_rgb::init()
{
    log_info("%s init run start\n", name);
    cfgEntry_t config[] = {
        {"bypass",                 UINT_32,     &this->bypass          }
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }

    hw_base::init();
    log_info("%s init run end\n", name);
}
cac_rgb::~cac_rgb()
{
    log_info("%s module deinit start\n", __FUNCTION__);
    if (cac_rgb_reg != NULL)
    {
        delete cac_rgb_reg;
    }
    log_info("%s module deinit end\n", __FUNCTION__);
}


void cac_rgb::checkparameters(cac_rgb_reg_t* reg)
{
    reg->bypass = common_check_bits(reg->bypass, 1, "bypass");

    log_info("================= cac rgb reg=================\n");
    log_info("bypass %d\n", reg->bypass);
    log_info("================= cac rgb reg=================\n");
}