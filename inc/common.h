#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "spdlog/spdlog.h"

enum bayer_type_t {
    BAYER_UNSUPPORT = -1,
    RGGB = 0,
    GRBG = 1,
    GBRG = 2,
    BGGR = 3
};

enum data_type_t {
    R = 0,
    G = 1,
    B = 2,
    Y = 3,
    U = 4,
    V = 5,
    RAW = 6,
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

        spdlog::info("alloc buffer {0:s} memory {1}", buffer_name, fmt::ptr(data_ptr));
    }
    ~data_buffer()
    {
        if (data_ptr != nullptr)
        {
            spdlog::info("free buffer {0:s} memory {1}", buffer_name, fmt::ptr(data_ptr));
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
    T min_val = 0;
    if (input > max_val)
    {
        spdlog::error("{} check fail max_val {}, current val {}", reg_name, max_val, input);
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
        spdlog::error("{} check fail max_val {}, current val {}", reg_name, max_val, input);
        return max_val;
    }
    else if (input < min_val)
    {
        spdlog::error("{} check fail min_val {}, current val {}", reg_name, max_val, input);
        return min_val;
    }
    return input;
}
