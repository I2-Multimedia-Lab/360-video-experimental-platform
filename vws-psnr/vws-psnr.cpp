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

    VWSPSNRMetric metric;
    if (!metric.LoadConfig(config)) {
        printf("Invalid config.\n");
        return -1;
    }

    if (!metric.Init()) {
        printf("Init failed.\n");
        return -1;
    }

    metric.Run();

    metric.Output();

    return 0;
}

