#include "stdafx.h"
#include "psnr_distortion.h"

PSNRDistortionMap::PSNRDistortionMap()
{

}

PSNRDistortionMap::~PSNRDistortionMap()
{

}

double PSNRDistortionMap::GetBlcokDistortion(const Rect& rc) const
{
    cv::Mat blockDiffMap(m_diffMap, rc);

    double MSE = (double)cv::sum(blockDiffMap)[0] / blockDiffMap.total();

    return MSE;
}

PSNRDistortion::PSNRDistortion()
{

}

PSNRDistortion::~PSNRDistortion()
{

}

PSNRDistortionMap* PSNRDistortion::Calculate(const Image& src, const Image& dst)
{
    assert(src.Format() == dst.Format());

    PSNRDistortionMap* distortionMap = new PSNRDistortionMap;
    cv::Mat& diffMap = distortionMap->m_diffMap;

    cv::absdiff(src.Data(), dst.Data(), diffMap);
    diffMap.convertTo(diffMap, CV_64FC1);
    cv::pow(diffMap, 2, diffMap);
    
    return distortionMap;
}
