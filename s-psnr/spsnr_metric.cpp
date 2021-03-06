#include "stdafx.h"
#include "video_source.h"
#include "spsnr_metric.h"

SPSNRMetric::SPSNRMetric()
    : m_numPoints(0)
    , m_globalPSNR(0.0)
    , m_duration(0.0)
{

}

SPSNRMetric::~SPSNRMetric()
{

}

bool SPSNRMetric::Init(const String& sphFile, int ifilter)
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

    m_mapper.SetIFilter(ifilter);

    return true;
}

bool SPSNRMetric::Calc(VideoSource& src, VideoSource& dst)
{
    double globalMSE = 0.0;
    m_duration = 0.0;

    int numFrames = src.FrameCount();
    for (int i = 0; i < numFrames; i++) {
        Image srcImg;
        if (!src.ReadNextFrame(srcImg))
            break;

        Image dstImg;
        if (!dst.ReadNextFrame(dstImg))
            break;

        clock_t start = clock();

        Vec1d srcPixels;
        m_mapper.SphPointFromImg(srcImg, src.Format(), m_points, srcPixels);

        Vec1d dstPixels;
        m_mapper.SphPointFromImg(dstImg, dst.Format(), m_points, dstPixels);

        double mse = MSE(srcPixels, dstPixels);
        double psnr = PSNR(mse);
        double t = (double)(clock() - start) / CLOCKS_PER_SEC;

        globalMSE += mse;
        m_duration += t;

        printf("Frame %d: %.4lf, %.4lf\n", i, psnr, t);
    }

    globalMSE /= numFrames;
    m_globalPSNR = PSNR(globalMSE);
    m_duration /= numFrames;

    return true;
}

double SPSNRMetric::MSE(Vec1d srcPixels, Vec1d dstPixels)
{
    assert(srcPixels.size() == dstPixels.size());

    size_t n = srcPixels.size();
    double totalMSE = 0.0;
    for (size_t i = 0; i < n; i++)
        totalMSE += pow(srcPixels[i] - dstPixels[i], 2.0);
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
    printf("Average Time: %lf\n", m_duration);
}
