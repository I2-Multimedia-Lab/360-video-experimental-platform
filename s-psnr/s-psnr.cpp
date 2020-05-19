// s-psnr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "config.h"
#include "video_source.h"
#include "spsnr_metric.h"

int main(int argc, char* argv[])
{
    Config cfg;
    if (!cfg.ParseCmdLineArgs(argc, argv)) {
        cfg.Usage();
        return -1;
    }

    VideoSource src;
    if (!src.Init(cfg.m_srcFile, cfg.m_srcWidth, cfg.m_srcHeight, cfg.m_srcFormat))
        return -1;

    VideoSource dst;
    if (!dst.Init(cfg.m_dstFile, cfg.m_dstWidth, cfg.m_dstHeight, cfg.m_dstFormat))
        return -1;

    SPSNRMetric metric;
    if (!metric.Init(cfg.m_sphFile, cfg.m_ifilter))
        return -1;

    if (!metric.Calc(src, dst))
        return -1;

    metric.Output();

    return 0;
}

