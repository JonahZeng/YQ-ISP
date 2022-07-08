#include "pipeline_manager.h"
#include "file_read.h"
#include "blc_hw.h"
#include "rgbir_remosaic_hw.h"
#include "demosaic_hw.h"


void test_bgrir_pipeline(pipeline_manager* manager)
{
    file_read* raw_in = new file_read(1, "raw_in");
    blc_hw* blc_hw_inst = new blc_hw(1, 1, "blc_hw");
    rgbir_remosaic_hw* rgbir_remosaic_hw_inst = new rgbir_remosaic_hw(1, 2, "rgbir_remosaic_hw");
    demosaic_hw* demosaic_hw_inst = new demosaic_hw(1, 3, "demosaic_hw");

    manager->register_hw_module(raw_in);
    manager->register_hw_module(blc_hw_inst);
    manager->register_hw_module(rgbir_remosaic_hw_inst);
    manager->register_hw_module(demosaic_hw_inst);

    manager->connect_port(raw_in, 0, blc_hw_inst, 0); //raw data
    manager->connect_port(blc_hw_inst, 0, rgbir_remosaic_hw_inst, 0); //raw data
    manager->connect_port(rgbir_remosaic_hw_inst, 0, demosaic_hw_inst, 0); //raw data
}