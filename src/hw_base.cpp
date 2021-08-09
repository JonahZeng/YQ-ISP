#include "hw_base.h"
#include <stdexcept>

hw_base::hw_base(uint32_t inpins, uint32_t outpins, const char* inst_name):
    in(inpins), out(outpins), previous_hw(inpins), outport_of_previous_hw(inpins), next_hw_of_outport(outpins), name(new char[64])
{
    this->inpins = inpins;
    this->outpins = outpins;
    size_t namelen = strlen(inst_name);
    if (name != nullptr) {
        if (namelen > 63)
        {
            memcpy_s(this->name, 63, inst_name, 63);
            this->name[63] = '\0';
        }
        else {
            memcpy_s(this->name, namelen, inst_name, namelen);
            this->name[namelen] = '\0';
        }
    }
    for (uint32_t i = 0; i < outpins; i++)
    {
        next_hw_of_outport[i].resize(0);
    }
}

void hw_base::connect_port(uint32_t out_port, hw_base* next_hw, uint32_t in_port)
{
    if (out_port >= this->outpins || in_port >= next_hw->inpins)
    {
        throw std::out_of_range("port out of range");
    }
    else {
        this->next_hw_of_outport[out_port].push_back(next_hw);
        next_hw->previous_hw[in_port] = this;
        next_hw->outport_of_previous_hw[in_port] = out_port;
    }
}

void hw_base::init()
{
    return;
}

void hw_base::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{
    return;
}

hw_base::~hw_base()
{
    if (name != nullptr)
    {
        delete[] name;
    }
    return;
}