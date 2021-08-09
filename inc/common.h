#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    data_buffer(uint32_t w, uint32_t h, data_type_t tp, const char* buf_name)
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

        size_t len = strlen(buf_name) + 1;
        buffer_name = new char[len];
        memcpy_s(buffer_name, len - 1, buf_name, len - 1);
        buffer_name[len - 1] = '\0';

        fprintf(stdout, "alloc buffer %s memory\n", buffer_name);
    }
    ~data_buffer()
    {
        if (data_ptr != nullptr)
        {
            delete[] data_ptr;
        }
        if (buffer_name != nullptr)
        {
            fprintf(stdout, "free buffer %s memory\n", buffer_name);
            delete[] buffer_name;
        }
    }
    uint32_t width;
    uint32_t height;
    uint16_t* data_ptr;
    data_type_t data_type;
    char* buffer_name;
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
