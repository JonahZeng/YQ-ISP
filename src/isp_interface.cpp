#include <stdio.h>
#include "pipeline_manager.h"

extern void test_V1_pipeline(pipeline_manager* manager);
extern void test_purple_fringing_pipe(pipeline_manager* manager);
extern void test_awbgain_pipeline(pipeline_manager* manager);

typedef struct _pipeline_collection {
    const char* pipeline_name;
    void(*f)(pipeline_manager* manager);
}pipeline_collection;

pipeline_collection pipelines[] = {
    {"test_V1_pipeline", test_V1_pipeline},//0
    {"test_purple_fringing_pipe", test_purple_fringing_pipe}, //1
    {"test_awbgain_pipeline", test_awbgain_pipeline} //2
};

static void print_usage()
{
    log_info("========================usage========================\n");
    log_info("-p   [pipe id],       select pipeline:\n");
    for(int32_t i=0; i<sizeof(pipelines)/sizeof(pipeline_collection); i++)
    {
        log_info("%d <-------> %s\n", i, pipelines[i].pipeline_name);
    }
    log_info("-cfg [cfg xml],       pipeline config file(.xml) path\n");
    log_info("-f   [frame number],  run pipeline times\n");
    log_info("-log [log file path], log2file\n");
    log_info("=====================================================\n");
}

bool run_isp(int32_t pipe_id, const char* log_fn, const char* cfg_file_name, int32_t frame_end)
{
    set_log_level(LOG_TRACE_LEVEL);
    if (pipe_id < 0 || frame_end < 0 || cfg_file_name == nullptr || strlen(cfg_file_name) == 0)
    {
        log_info("param error\n");
        print_usage();
        return false;
    }
    if (strlen(cfg_file_name) == 0)
    {
        log_info("param error\n");
        print_usage();
        return false;
    }

    if (pipe_id >= sizeof(pipelines) / sizeof(pipeline_collection))
    {
        log_error("pipe_id out of range\n");
        print_usage();
        return false;
    }

    if(log_fn && strlen(log_fn) > 0)
    {
        open_log_file(log_fn);
    }

    log_info("run command: -p %d -cfg %s -f %d -log %s\n", pipe_id, cfg_file_name, frame_end, log_fn);

    pipeline_manager* isp_pipe_manager = new pipeline_manager(); //use heap just for log2file, log pipeline destruct
    isp_pipe_manager->frames = frame_end;
    pipelines[pipe_id].f(isp_pipe_manager);

    isp_pipe_manager->cfg_file_name = std::string(cfg_file_name);
    isp_pipe_manager->init();
    isp_pipe_manager->read_xml_cfg();

    for (int fm = 0; fm < frame_end; fm++)
    {
        isp_pipe_manager->run(isp_pipe_manager->stat_addr, fm);
    }
    delete isp_pipe_manager;
    
    log_info("========process end.=========\n");
    close_log_file();

    return true;
}