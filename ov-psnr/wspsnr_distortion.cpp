#include "stdafx.h"
#include "wspsnr_distortion.h"

WSPSNRDistortionMap::WSPSNRDistortionMap(const cv::Mat& weightMap)
    : m_weightMap(weightMap)
{

}

WSPSNRDistortionMap::~WSPSNRDistortionMap()
{

}

double WSPSNRDistortionMap::GetBlcokDistortion(const Rect& rc) const
{
    cv::Mat blockDiffMap(m_weightedDiffMap, rc);
    cv::Mat blockWeightMap(m_weightMap, rc);

    double WMSE = cv::sum(blockDiffMap)[0] / cv::sum(blockWeightMap)[0];

    return WMSE;
}

WSPSNRDistortion::WSPSNRDistortion()
{

}

WSPSNRDistortion::~WSPSNRDistortion()
{

}

WSPSNRDistortionMap* WSPSNRDistortion::Calculate(const Image& src, const Image& dst)
{
    assert(src.Format() == dst.Format());

    GenerateWeightMap(src.Width(), src.Height());

    WSPSNRDistortionMap* distortionMap = new WSPSNRDistortionMap(m_weightMap);
    cv::Mat& weightedDiffMap = distortionMap->m_weightedDiffMap;

    cv::absdiff(src.Data(), dst.Data(), weightedDiffMap);
    weightedDiffMap.convertTo(weightedDiffMap, CV_64FC1);
    cv::pow(weightedDiffMap, 2, weightedDiffMap);

    weightedDiffMap = weightedDiffMap.mul(m_weightMap);

    return distortionMap;
}

void WSPSNRDistortion::GenerateWeightMap(int width, int height)
{
    if (!m_weightMap.empty())
        return;

    m_weightMap.ones(height, width, CV_64FC1);

    // equirectangular weight from WS-PSNR
    for (int j = 0; j < m_weightMap.rows; j++) {
        double weight = cos((j - (height / 2 - 0.5)) * M_PI / height);
        m_weightMap.row(j) = m_weightMap.row(j) * weight;
    }
}
