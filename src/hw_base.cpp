#include "hw_base.h"
#include <typeinfo>
#include <memory>
#include <stdexcept>
#include "fileRead.h"

hw_base::hw_base(uint32_t inpins, uint32_t outpins, const char* inst_name) :
    in(inpins), out(outpins), previous_hw(inpins), outport_of_previous_hw(inpins),
    next_hw_of_outport(outpins), next_hw_cnt_of_outport(outpins), name(new char[64]), write_pic_src_pin(), input_file_name(nullptr)
{
    this->inpins = inpins;
    this->outpins = outpins;
    size_t namelen = strlen(inst_name);
    if (name != nullptr) {
        if (namelen > 63)
        {
            memcpy(this->name, inst_name, 63);
            this->name[63] = '\0';
        }
        else {
            memcpy(this->name, inst_name, namelen);
            this->name[namelen] = '\0';
        }
    }
    for (uint32_t i = 0; i < outpins; i++)
    {
        next_hw_of_outport[i].resize(0);
        next_hw_cnt_of_outport[i] = 0;
    }

    for (uint32_t in_p = 0; in_p < inpins; in_p++)
    {
        in[in_p] = nullptr;
    }

    for (uint32_t out_p = 0; out_p < outpins; out_p++)
    {
        out[out_p] = nullptr;
    }

    write_pic = false;
    write_pic_bits = 0;
    write_pic_format[0] = 'R';
    write_pic_format[1] = 'A';
    write_pic_format[2] = 'W';
    write_pic_format[3] = '\0';

    memset(write_pic_path, 0, 256);
}

void hw_base::hw_init()
{
    cfgEntry_t config[] = {
        {"xmlConfigValid",     BOOL_T,           &this->xmlConfigValid             },
        {"write_pic",          BOOL_T,           &this->write_pic                  },
        {"write_pic_src_pin",  VECT_UINT32,      &this->write_pic_src_pin,        3},
        {"write_pic_format",   STRING,           this->write_pic_format,         16},
        {"write_pic_path",     STRING,           this->write_pic_path,          256},
        {"write_pic_bits",     UINT_32,          &this->write_pic_bits,            },
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->hwCfgList.push_back(config[i]);
    }
}

void hw_base::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    for (uint32_t in_p = 0; in_p < inpins; in_p++)
    {
        hw_base* pre_hw = this->previous_hw[in_p];
        uint32_t pre_hw_port = this->outport_of_previous_hw[in_p];
        if (pre_hw->next_hw_cnt_of_outport[pre_hw_port] > 0)
        {
            pre_hw->next_hw_cnt_of_outport[pre_hw_port] -= 1;
        }

        if (pre_hw->next_hw_cnt_of_outport[pre_hw_port] == 0)
        {
            if (pre_hw->out[pre_hw_port] != nullptr)
            {
                if (typeid(*pre_hw) != typeid(fileRead))
                {
                    delete pre_hw->out[pre_hw_port];
                    pre_hw->out[pre_hw_port] = nullptr;
                }
            }
        }
    }

    if (this->write_pic)
    {
        input_file_name = &(stat_out->input_file_name);
        write_pic_for_output();
    }
}

void hw_base::reset_hw_cnt_of_outport()
{
    for (uint32_t out_port = 0; out_port < outpins; out_port++)
    {
        this->next_hw_cnt_of_outport[out_port] = (uint32_t)this->next_hw_of_outport[out_port].size();
    }
}

void hw_base::release_output_memory()
{
    for (uint32_t out_port = 0; out_port < outpins; out_port++)
    {
        if (this->out[out_port] != nullptr)
        {
            delete this->out[out_port];
            this->out[out_port] = nullptr;
        }
    }
}

bool hw_base::prepare_input()
{
    for (uint32_t in_p = 0; in_p < inpins; in_p++)
    {
        hw_base* pre_hw = this->previous_hw[in_p];
        uint32_t pre_hw_out_port = this->outport_of_previous_hw[in_p];
        if (pre_hw_out_port >= pre_hw->out.size())
        {
            throw std::out_of_range("port out of range");
        }
        in[in_p] = pre_hw->out[pre_hw_out_port];
    }

    for (uint32_t in_p = 0; in_p < inpins; in_p++)
    {
        if (in[in_p] == nullptr)
            return false;
    }
    return true;
}

void hw_base::write_raw_for_output(FILE* fp)
{
    uint32_t port = write_pic_src_pin[0];
    if (port >= out.size())
    {
        log_error("can't write pic because port = %s, all outport size = %ld\n", port, out.size());
        return;
    }
    data_buffer* src_data = out[port];
    if (src_data != nullptr)
    {
        if (write_pic_bits > 8 && write_pic_bits <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_ptr(new uint16_t[src_data->width * src_data->height]);
            uint16_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data->width * src_data->height; sz++)
            {
                buffer[sz] = (src_data->data_ptr)[sz] >> (16 - write_pic_bits);
            }
            fwrite(buffer, sizeof(uint16_t), src_data->width * src_data->height, fp);
        }
        else if (write_pic_bits <= 8)
        {
            std::unique_ptr<uint8_t[]> buffer_ptr(new uint8_t[src_data->width * src_data->height]);
            uint8_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data->width * src_data->height; sz++)
            {
                buffer[sz] = (src_data->data_ptr)[sz] >> (16 - write_pic_bits);
            }
            fwrite(buffer, sizeof(uint8_t), src_data->width * src_data->height, fp);
        }
    }
}

void hw_base::write_pnm_for_output(FILE* fp)
{
    uint32_t port0 = write_pic_src_pin[0];
    uint32_t port1 = write_pic_src_pin[1];
    uint32_t port2 = write_pic_src_pin[2];
    if (port0 >= out.size() || port1 >= out.size() || port2 >= out.size())
    {
        log_error("can't write pic because port = %d %d %d, all outport size = %ld\n", port0, port1, port2, out.size());
        return;
    }

    data_buffer* src_data0 = out[port0];
    data_buffer* src_data1 = out[port1];
    data_buffer* src_data2 = out[port2];

    if (src_data0 != nullptr && src_data1 != nullptr && src_data2 != nullptr
        && src_data0->width == src_data1->width && src_data1->width == src_data2->width
        && src_data0->height == src_data1->height && src_data1->height == src_data2->height)
    {
        char header0[64] = { 0 };

        int len = sprintf(header0, "P6\n%d %d\n%d\n", src_data0->width, src_data0->height, (1U << write_pic_bits) - 1);

        if (strlen(header0) % 2 == 1)
        {
            len = sprintf(header0, "P6\n%d  %d\n%d\n", src_data0->width, src_data0->height, (1U << write_pic_bits) - 1);
        }
        fwrite(header0, sizeof(char), strlen(header0), fp);

        if (write_pic_bits > 8 && write_pic_bits <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_ptr(new uint16_t[src_data0->width * src_data0->height * 3]);
            uint16_t* buffer = buffer_ptr.get();
            uint16_t tmp;
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height; sz++)
            {
                tmp = ((src_data0->data_ptr)[sz]) >> (16 - write_pic_bits);
                tmp = ((tmp & 0x00ff) << 8) | ((tmp & 0xff00) >> 8);
                buffer[sz * 3 + 0] = tmp;

                tmp = ((src_data1->data_ptr)[sz]) >> (16 - write_pic_bits);
                tmp = ((tmp & 0x00ff) << 8) | ((tmp & 0xff00) >> 8);
                buffer[sz * 3 + 1] = tmp;

                tmp = ((src_data2->data_ptr)[sz]) >> (16 - write_pic_bits);
                tmp = ((tmp & 0x00ff) << 8) | ((tmp & 0xff00) >> 8);
                buffer[sz * 3 + 2] = tmp;
            }
            fwrite(buffer, sizeof(uint16_t), src_data0->width * src_data0->height * 3, fp);
        }
        else if (write_pic_bits <= 8)
        {
            std::unique_ptr<uint8_t[]> buffer_ptr(new uint8_t[src_data0->width * src_data0->height * 3]);
            uint8_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height; sz++)
            {
                buffer[sz * 3 + 0] = ((src_data0->data_ptr)[sz]) >> (16 - write_pic_bits);
                buffer[sz * 3 + 1] = ((src_data1->data_ptr)[sz]) >> (16 - write_pic_bits);
                buffer[sz * 3 + 2] = ((src_data2->data_ptr)[sz]) >> (16 - write_pic_bits);
            }
            fwrite(buffer, sizeof(uint8_t), src_data0->width * src_data0->height * 3, fp);
        }
    }
}

void hw_base::write_yuv422_for_output(FILE* fp)//UYVY
{
    uint32_t port0 = write_pic_src_pin[0];
    uint32_t port1 = write_pic_src_pin[1];
    uint32_t port2 = write_pic_src_pin[2];
    if (port0 >= out.size() || port1 >= out.size() || port2 >= out.size())
    {
        log_error("can't write pic because port = %d %d %d, all outport size = %ld\n", port0, port1, port2, out.size());
        return;
    }

    data_buffer* src_data0 = out[port0];
    data_buffer* src_data1 = out[port1];
    data_buffer* src_data2 = out[port2];
    if (src_data0 != nullptr && src_data1 != nullptr && src_data2 != nullptr &&
        src_data0->width == 2 * src_data1->width && src_data0->width == 2 * src_data2->width &&
        src_data0->height == src_data1->height && src_data0->height == src_data2->height)
    {
        if (write_pic_bits > 8 && write_pic_bits <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_ptr(new uint16_t[src_data0->width * 2 * src_data0->height]);
            uint16_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height * 2; sz++)
            {
                if (sz % 2 == 1)
                {
                    buffer[sz] = (src_data0->data_ptr)[sz / 2] >> (16 - write_pic_bits);
                }
                else if (sz % 4 == 0)
                {
                    buffer[sz] = (src_data1->data_ptr)[sz / 4] >> (16 - write_pic_bits);
                }
                else if (sz % 4 == 2)
                {
                    buffer[sz] = (src_data2->data_ptr)[sz / 4] >> (16 - write_pic_bits);
                }
            }
            fwrite(buffer, sizeof(uint16_t), src_data0->width * 2 * src_data0->height, fp);
        }
        else if (write_pic_bits <= 8)
        {
            std::unique_ptr<uint8_t[]> buffer_ptr(new uint8_t[src_data0->width * 2 * src_data0->height]);
            uint8_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height * 2; sz++)
            {
                if (sz % 2 == 1)
                {
                    buffer[sz] = (src_data0->data_ptr)[sz / 2] >> (16 - write_pic_bits);
                }
                else if (sz % 4 == 0)
                {
                    buffer[sz] = (src_data1->data_ptr)[sz / 4] >> (16 - write_pic_bits);
                }
                else if (sz % 4 == 2)
                {
                    buffer[sz] = (src_data2->data_ptr)[sz / 4] >> (16 - write_pic_bits);
                }
            }
            fwrite(buffer, sizeof(uint8_t), src_data0->width * 2 * src_data0->height, fp);
        }
    }
}

void hw_base::write_yuv444_for_output(FILE* fp)//YUV
{
    uint32_t port0 = write_pic_src_pin[0];
    uint32_t port1 = write_pic_src_pin[1];
    uint32_t port2 = write_pic_src_pin[2];
    if (port0 >= out.size() || port1 >= out.size() || port2 >= out.size())
    {
        log_error("can't write pic because port = %d %d %d, all outport size = %ld\n", port0, port1, port2, out.size());
        return;
    }

    data_buffer* src_data0 = out[port0];
    data_buffer* src_data1 = out[port1];
    data_buffer* src_data2 = out[port2];
    if (src_data0 != nullptr && src_data1 != nullptr && src_data2 != nullptr &&
        src_data0->width == src_data1->width && src_data0->width == src_data2->width &&
        src_data0->height == src_data1->height && src_data0->height == src_data2->height)
    {
        if (write_pic_bits > 8 && write_pic_bits <= 16)
        {
            std::unique_ptr<uint16_t[]> buffer_ptr(new uint16_t[src_data0->width *src_data0->height * 3]);
            uint16_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height; sz++)
            {
                buffer[sz * 3 + 0] = (src_data0->data_ptr)[sz] >> (16 - write_pic_bits);
                buffer[sz * 3 + 1] = (src_data1->data_ptr)[sz] >> (16 - write_pic_bits);
                buffer[sz * 3 + 2] = (src_data2->data_ptr)[sz] >> (16 - write_pic_bits);
            }
            fwrite(buffer, sizeof(uint16_t), src_data0->width *src_data0->height * 3, fp);
        }
        else if (write_pic_bits <= 8)
        {
            std::unique_ptr<uint8_t[]> buffer_ptr(new uint8_t[src_data0->width *src_data0->height * 3]);
            uint8_t* buffer = buffer_ptr.get();
            for (uint32_t sz = 0; sz < src_data0->width * src_data0->height; sz++)
            {
                buffer[sz * 3 + 0] = (src_data0->data_ptr)[sz] >> (16 - write_pic_bits);
                buffer[sz * 3 + 1] = (src_data1->data_ptr)[sz] >> (16 - write_pic_bits);
                buffer[sz * 3 + 2] = (src_data2->data_ptr)[sz] >> (16 - write_pic_bits);
            }
            fwrite(buffer, sizeof(uint8_t), src_data0->width *src_data0->height * 3, fp);
        }
    }
}

void hw_base::write_pic_for_output()
{
    if (write_pic_src_pin.size() > out.size())
    {
        log_error("can't write pic because outpins = %ld, and xml config pins for pic = %ld\n", out.size(), write_pic_src_pin.size());
        return;
    }
    FILE* fp = nullptr;

    std::string write_pic_path_str(write_pic_path);

    if (input_file_name != nullptr && input_file_name->length() > 0)
    {
        size_t ridx1 = input_file_name->rfind('/');
        size_t ridx2 = input_file_name->rfind('.');
        std::string input_raw_fn = input_file_name->substr(ridx1 + 1, ridx2 - ridx1 - 1);

        ridx1 = write_pic_path_str.rfind('/');
        write_pic_path_str.insert(ridx1 + 1, input_raw_fn.c_str(), input_raw_fn.size());
    }
    if (strcmp(write_pic_format, "RAW") == 0 && write_pic_src_pin.size() == 1)
    {
        fp = fopen(write_pic_path_str.c_str(), "wb");

        if (fp == nullptr)
        {
            log_error("open file %s fail\n", write_pic_path_str.c_str());
            return;
        }
        write_raw_for_output(fp);
    }
    else if (strcmp(write_pic_format, "PNM") == 0 && write_pic_src_pin.size() == 3)
    {
        fp = fopen(write_pic_path_str.c_str(), "wb");
        if (fp == nullptr)
        {
            log_error("open file %s fail\n", write_pic_path_str.c_str());
            return;
        }
        write_pnm_for_output(fp);
    }
    else if (strcmp(write_pic_format, "YUV422") == 0 && write_pic_src_pin.size() == 3)
    {
        fp = fopen(write_pic_path_str.c_str(), "wb");
        if (fp == nullptr)
        {
            log_error("open file %s fail\n", write_pic_path_str.c_str());
            return;
        }
        write_yuv422_for_output(fp);
    }
    else if (strcmp(write_pic_format, "YUV444") == 0 && write_pic_src_pin.size() == 3)
    {
        fp = fopen(write_pic_path_str.c_str(), "wb");
        if (fp == nullptr)
        {
            log_error("open file %s fail\n", write_pic_path_str.c_str());
            return;
        }
        write_yuv444_for_output(fp);
    }
    else {
        log_error("can't write pic, RAW:1, PNM:3, YUV:3\n");
    }

    if (fp)
    {
        fclose(fp);
    }
}

hw_base::~hw_base()
{
    if (name != nullptr)
    {
        log_info("deinit module %s\n", name);
        delete[] name;
    }
}