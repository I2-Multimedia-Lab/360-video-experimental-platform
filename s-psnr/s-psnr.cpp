// s-psnr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "option.h"
#include "video_source.h"
#include "spsnr_metric.h"

int main(int argc, char* argv[])
{
    Option opt;
    if (!opt.ParseCmdLineArgs(argc, argv)) {
        opt.Usage();
        return -1;
    }

    VideoSource src;
    if (!src.Init(opt.m_srcFile, opt.m_srcWidth, opt.m_srcHeight, opt.m_srcFormat))
        return -1;

    VideoSource dst;
    if (!dst.Init(opt.m_dstFile, opt.m_dstWidth, opt.m_dstHeight, opt.m_dstFormat))
        return -1;

    SPSNRMetric metric;
    if (!metric.Init(opt.m_sphFile))
        return -1;

    if (!metric.Calc(src, dst))
        return -1;

    metric.Output();

    return 0;
}

