#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

enum bayer_type_t {
    BAYER_UNSUPPORT = -1,
    RGGB = 0,
    GRBG = 1,
    GBRG = 2,
    BGGR = 3
};

enum pixel_bayer_type{
    PIX_R = 0,
    PIX_GR = 1,
    PIX_GB = 2,
    PIX_B = 3
};

enum data_type_t {
    DATA_TYPE_R = 0,
    DATA_TYPE_G = 1,
    DATA_TYPE_B = 2,
    DATA_TYPE_Y = 3,
    DATA_TYPE_U = 4,
    DATA_TYPE_V = 5,
    DATA_TYPE_RAW = 6,
    DATA_TYPE_UNSUPPORT = -1
};

enum file_type_t {
    RGB_FORMAT = 0,
    YUV444_FORMAT = 1,
    YUV422_FORMAT = 2,
    YUV420_FORMAT =3,
    RAW_FORMAT = 4,
    DNG_FORMAT = 5,
    FILE_TYPE_UNSUPPORT = -1
};

struct data_buffer {
    data_buffer(const data_buffer& src) = delete;
    data_buffer() = delete;
    data_buffer& operator=(const data_buffer& src) = delete;
    data_buffer(uint32_t w, uint32_t h, data_type_t tp, bayer_type_t by, const char* buf_name)
    {
        data_ptr = new uint16_t[w*h];
        if (data_ptr != nullptr)
        {
            width = w;
            height = h;
        }
        else {
            width = 0;
            height = 0;
        }
        data_type = tp;
        bayer_pattern = by;

        size_t len = strlen(buf_name) + 1;
        buffer_name = new char[len];
#ifdef _MSC_VER
        memcpy_s(buffer_name, len - 1, buf_name, len - 1);
#else
        memcpy(buffer_name, buf_name, len - 1);
#endif
        buffer_name[len - 1] = '\0';

        log_debug("alloc buffer %s memory %p\n", buffer_name, data_ptr);
    }
    ~data_buffer()
    {
        if (data_ptr != nullptr)
        {
            log_debug("free buffer %s memory %p\n", buffer_name, data_ptr);
            delete[] data_ptr;
        }
        if (buffer_name != nullptr)
        {
            delete[] buffer_name;
        }
    }
    uint32_t width;
    uint32_t height;
    uint16_t* data_ptr;
    data_type_t data_type;
    char* buffer_name;
    bayer_type_t bayer_pattern;
};

enum cfg_data_type {
    BOOL_T,
    INT_32,
    UINT_32,
    VECT_INT32,
    VECT_UINT32,
    STRING
};

typedef struct cfgEntry_s {
    const char* tagName;
    cfg_data_type type;
    void* targetAddr;
    size_t max_len;
}cfgEntry_t;


template<typename T>
T common_check_bits(T& input, uint32_t bits, const char* reg_name) //for unsigned type
{
    T max_val = (1U << bits) - 1;
    T min_val = 0U;
    if (input > max_val)
    {
        log_error("%s check fail max_val %d, current val %d\n", reg_name, max_val, input);
        return max_val;
    }
    return input;
}

template<typename T>
T common_check_bits_ex(T& input, uint32_t bits, const char* reg_name) //for signed type
{
    T max_val = (1 << (bits - 1)) - 1;
    T min_val = 0 - (1 << (bits - 1));
    if (input > max_val)
    {
        log_error("%s check fail max_val %d, current val %d\n", reg_name, max_val, input);
        return max_val;
    }
    else if (input < min_val)
    {
        log_error("%s check fail min_val %d, current val %d\n", reg_name, min_val, input);
        return min_val;
    }
    return input;
}
