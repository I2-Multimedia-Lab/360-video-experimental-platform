#pragma once

#include "mapper.h"

class SPSNRMetric
{
public:
    SPSNRMetric();
    ~SPSNRMetric();

    bool Init(const String& sphFile, int ifilter);
    bool Calc(VideoSource& src, VideoSource& dst);
    void Output();

private:
    double MSE(Vec1d srcPixels, Vec1d dstPixels);
    double PSNR(double mse);

private:
    int m_numPoints;
    Vec2f m_points;
    double m_globalPSNR;
    Mapper m_mapper;
};
