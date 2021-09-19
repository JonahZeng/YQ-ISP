/*
pipeline 构造

*/
#include "pipeline_manager.h"
#include "fileRead.h"

void test_V1_pipeline(pipeline_manager* manager)
{
    fileRead* fileRead_inst = new fileRead(1, "raw_in");

    manager->register_module(fileRead_inst);
}


