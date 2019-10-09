#include "stdafx.h"
#include "config.h"
#include "metric.h"

int main(int argc, char* argv[])
{
    Config cfg;
    if (!cfg.ParseCmdLineArgs(argc, argv))
        return -1;

    Metric metric;
    if (!metric.Init(cfg))
        return -1;

    metric.Compare();

    metric.Output();

    return 0;
}
