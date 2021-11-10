#include <stdio.h>
#include <string>
#include "pipeline_manager.h"

extern void test_V1_pipeline(pipeline_manager* manager);
extern void test_purple_fringing_pipe(pipeline_manager* manager);

typedef struct _pipeline_collection {
    const char* pipeline_name;
    void(*f)(pipeline_manager* manager);
}pipeline_collection;

pipeline_collection pipelines[] = {
    {"test_V1_pipeline", test_V1_pipeline},//0
    {"test_purple_fringing_pipe", test_purple_fringing_pipe} //1
};

// -p pipe_number -cfg xml_file -f frame_end 
int main(int argc, char* argv[])
{
    set_log_level(LOG_TRACE_LEVEL);
    if (argc < 7) {
        log_info("input param number must >= 5\n");
        return 0;
    }
    log_info("run command:\n");

    std::string cmd_str;
    for (int i = 0; i < argc; i++)
    {
        cmd_str.append(argv[i]);
        cmd_str.append(" ");
    }
    log_info("%s\n", cmd_str.c_str());

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
        log_info("param error\n");
        return 0;
    }

    if (pipe_id >= sizeof(pipelines) / sizeof(pipeline_collection))
    {
        log_error("pipe_id out of range\n");
        return 0;
    }

    pipeline_manager isp_pipe_manager;
    isp_pipe_manager.frames = frame_end;
    pipelines[pipe_id].f(&isp_pipe_manager);

    isp_pipe_manager.init();
    isp_pipe_manager.read_xml_cfg(cfg_file_name);

    for (int fm = 0; fm < frame_end; fm++)
    {
        isp_pipe_manager.run(isp_pipe_manager.stat_addr, fm);
    }

    return 0;
}