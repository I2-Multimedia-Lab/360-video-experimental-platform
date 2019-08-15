#include "stdafx.h"
#include "video_source.h"
#include "spsnr_metric.h"

SPSNRMetric::SPSNRMetric()
    : m_numPoints(0)
    , m_globalPSNR(0.0)
{

}

SPSNRMetric::~SPSNRMetric()
{

}

bool SPSNRMetric::Init(const String& sphFile)
{
    char tmp[32];
    if (sscanf(sphFile.c_str(), "%[^_]_%d.txt", tmp, &m_numPoints) != 2)
        return false;

    FILE* fp = fopen(sphFile.c_str(), "r");
    if (fp == nullptr) {
        printf("sph_file open failed!\n");
        return false;
    }
    
    m_points.resize(m_numPoints);

    for (int i = 0; i < m_numPoints; i++)
        fscanf(fp, "%f %f", &m_points[i].x, &m_points[i].y);

    fclose(fp);

    return true;
}

bool SPSNRMetric::Calc(VideoSource& src, VideoSource& dst)
{
    double globalMSE = 0.0;

    int numFrames = src.FrameCount();
    for (int i = 0; i < numFrames; i++) {
        Image srcImg;
        if (!src.ReadNextFrame(srcImg))
            break;

        Image dstImg;
        if (!dst.ReadNextFrame(dstImg))
            break;

        Vec1b srcPixels;
        m_mapper.SphPointFromImg(srcImg, src.Format(), m_points, srcPixels);

        Vec1b dstPixels;
        m_mapper.SphPointFromImg(dstImg, dst.Format(), m_points, dstPixels);

        double mse = MSE(srcPixels, dstPixels);
        globalMSE += mse;

        printf("Frame %d: %lf\n", i, PSNR(mse));
    }

    globalMSE /= numFrames;
    m_globalPSNR = PSNR(globalMSE);

    return true;
}

double SPSNRMetric::MSE(Vec1b srcPixels, Vec1b dstPixels)
{
    assert(srcPixels.size() == dstPixels.size());

    size_t n = srcPixels.size();
    double totalMSE = 0.0;
    for (size_t i = 0; i < n; i++)
        totalMSE += pow((double)srcPixels[i] - (double)dstPixels[i], 2.0);
    totalMSE /= n;

    return totalMSE;
}

double SPSNRMetric::PSNR(double mse)
{
    return 10 * log10(255 * 255 / (mse + DBL_EPSILON));
}

void SPSNRMetric::Output()
{
    printf("Global PSNR: %lf\n", m_globalPSNR);
}
