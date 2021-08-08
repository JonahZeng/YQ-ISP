#include "fileRead.h"

fileRead::fileRead(uint32_t outpins, char* inst_name):hw_base(0, outpins, inst_name), file_name()
{
    bayer = RGGB;
    bit_depth = 10;
    file_type = RAW_FORMAT;
}

void fileRead::hw_run(statistic_info_t* stat_out, uint32_t frame_cnt)
{

}

void fileRead::init()
{
    cfgEntry_t config[] = {
        {"bayer_type", STRING, this->bayer_string},
        {"bit_depth", UINT_32, &this->bit_depth},
        {"file_type", STRING, this->file_type_string},
        {"file_name", STRING, this->file_name}
    };
    for (int i = 0; i < sizeof(config) / sizeof(cfgEntry_t); i++)
    {
        this->cfgList.push_back(config[i]);
    }
}

fileRead::~fileRead()
{

}