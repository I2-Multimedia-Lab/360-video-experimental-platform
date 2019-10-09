#pragma once

#include "common.h"

class WSPSNRDistortionMap : public DistortionMap
{
public:
    friend class WSPSNRDistortion;

    WSPSNRDistortionMap(const cv::Mat& weightMap);
    virtual ~WSPSNRDistortionMap();

    virtual double GetBlcokDistortion(const Rect& rc) const;

private:
    cv::Mat m_weightedDiffMap;
    const cv::Mat& m_weightMap;
};

class WSPSNRDistortion
{
public:
    WSPSNRDistortion();
    ~WSPSNRDistortion();

    WSPSNRDistortionMap* Calculate(const Image& src, const Image& dst);

private:
    void GenerateWeightMap(int width, int height);

private:
    cv::Mat m_weightMap;
};

