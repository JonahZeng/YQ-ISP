#pragma once

#include <stdint.h>

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
    RGB = 0,
    YUV444 = 1,
    YUV422 = 2,
    YUV420 =3,
    RAW_FORMAT = 4,
    FILE_TYPE_UNSUPPORT = -1
};

struct data_buffer {
    data_buffer(uint32_t w, uint32_t h)
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
    }
    ~data_buffer()
    {
        if (data_ptr != nullptr)
        {
            delete[] data_ptr;
        }
    }
    uint32_t width;
    uint32_t height;
    uint16_t* data_ptr;
    data_type_t data_type;
};

enum cfg_data_type {
    BOOL,
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
}cfgEntry_t;
