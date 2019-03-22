// vws-psnr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vws_psnr_metric.h"


int main(int argc, char* argv[])
{
    VWSPSNRConfig config;
    if (!config.ParseFromCmdLineArgs(argc, argv)) {
        VWSPSNRConfig::Usage();
        return -1;
    }

    VWSPSNRMetric vws_pnsr;
    if (!vws_pnsr.LoadConfig(config)) {
        printf("Invalid config.\n");
        return -1;
    }

    if (!vws_pnsr.Init()) {
        printf("Init failed.\n");
        return -1;
    }

    vws_pnsr.Run();

    return 0;
}

