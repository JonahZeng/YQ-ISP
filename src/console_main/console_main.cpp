#include "isp_interface.h"
#include <cstring>
#include <cstdlib>

int main(int argc, char* argv[])
{
    int32_t pipe_id = -1;
    char* cfg_file_name = nullptr;
    int32_t frame_end = -1;
    char* log_fn = nullptr;

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
         if (strcmp("-log", argv[i]) == 0 && i < (argc - 1))
        {
            log_fn = argv[i + 1];
        }
    }
    bool ret = run_isp(pipe_id, log_fn, cfg_file_name, frame_end);
    if(ret)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
