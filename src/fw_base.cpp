#include "fw_base.h"

fw_base::fw_base(uint32_t inpins, uint32_t outpins, const char* inst_name):fwCfgList(0), name(new char[64])
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
}

fw_base::~fw_base()
{
    if (name != nullptr)
    {
        log_info("deinit module %s\n", name);
        delete[] name;
    }
}