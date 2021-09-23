#include "hw_base.h"
#include <stdexcept>
#include "spdlog/spdlog.h"

hw_base::hw_base(uint32_t inpins, uint32_t outpins, const char* inst_name):
    in(inpins), out(outpins), previous_hw(inpins), outport_of_previous_hw(inpins), 
    next_hw_of_outport(outpins), next_hw_cnt_of_outport(outpins), name(new char[64]), write_pic_src_pin()
{
    this->inpins = inpins;
    this->outpins = outpins;
    size_t namelen = strlen(inst_name);
    if (name != nullptr) {
        if (namelen > 63)
        {
#ifdef _MSC_VER
            memcpy_s(this->name, 63, inst_name, 63);
#else
            memcpy(this->name, inst_name, 63);
#endif
            this->name[63] = '\0';
        }
        else {
#ifdef _MSC_VER
            memcpy_s(this->name, namelen, inst_name, namelen);
#else
            memcpy(this->name, inst_name, namelen);
#endif
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

void hw_base::connect_port(uint32_t out_port, hw_base* next_hw, uint32_t in_port)
{
    if (out_port >= this->outpins || in_port >= next_hw->inpins)
    {
        throw std::out_of_range("port out of range");
    }
    else {
        this->next_hw_of_outport[out_port].push_back(next_hw);
        this->next_hw_cnt_of_outport[out_port]++;
        next_hw->previous_hw[in_port] = this;
        next_hw->outport_of_previous_hw[in_port] = out_port;
    }
}

void hw_base::init()
{
    spdlog::info("{0} run start", __FUNCTION__);

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
        this->cfgList.push_back(config[i]);
    }

    spdlog::info("{0} run end", __FUNCTION__);
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
                delete pre_hw->out[pre_hw_port];
                pre_hw->out[pre_hw_port] = nullptr;
            }
        }
    }
    return;
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

void hw_base::write_pic_for_output()
{

}

hw_base::~hw_base()
{
    if (name != nullptr)
    {
        spdlog::info("deinit module {0}", name);
        delete[] name;
    }
    return;
}