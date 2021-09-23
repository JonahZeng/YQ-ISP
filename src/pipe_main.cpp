#include <stdio.h>
#include "pipeline_manager.h"
#include "spdlog/spdlog.h"

extern void test_V1_pipeline(pipeline_manager* manager);

typedef struct _pipeline_collection {
    const char* pipeline_name;
    void(*f)(pipeline_manager* manager);
}pipeline_collection;

pipeline_collection pipelines[] = {
    {"test_V1_pipeline", test_V1_pipeline}
};

// -p pipe_number -cfg xml_file -f frame_end 
int main(int argc, char* argv[])
{
    //spdlog::logger* logger = spdlog::default_logger_raw();
    if (argc < 7) {
        spdlog::info("input param number must >= 5");
        return 0;
    }
    spdlog::info("run command:");

    std::string cmd_str;
    for (int i = 0; i < argc; i++)
    {
        cmd_str.append(argv[i]);
        cmd_str.append(" ");
    }
    spdlog::info(cmd_str);

    int32_t pipe_id = -1;
    char* cfg_file_name = NULL;
    int32_t frame_end = -1;

    for (int32_t i = 1; i < argc; i++)
    {
        if (strcmp("-p", argv[i]) == 0 && i < (argc - 1))
        {
            pipe_id = atoi(argv[i + 1]);
        }
        if (strcmp("-cfg", argv[i]) == 0 && i < (argc - 1))
        {
            cfg_file_name = argv[i + 1];
        }
        if (strcmp("-f", argv[i]) == 0 && i < (argc - 1))
        {
            frame_end = atoi(argv[i + 1]);
        }
    }
    if (pipe_id < 0 || frame_end < 0 || cfg_file_name == NULL)
    {
        spdlog::info("param error");
        return 0;
    }

    if (pipe_id >= sizeof(pipelines) / sizeof(pipeline_collection))
    {
        spdlog::error("pipe_id out of range");
        return 0;
    }

    pipeline_manager isp_pipe_manager;
    pipelines[pipe_id].f(&isp_pipe_manager);

    isp_pipe_manager.init();
    isp_pipe_manager.read_xml_cfg(cfg_file_name);

    for (int fm = 0; fm < frame_end; fm++)
    {
        isp_pipe_manager.run(isp_pipe_manager.stat_addr, fm);
    }

    return 0;
}