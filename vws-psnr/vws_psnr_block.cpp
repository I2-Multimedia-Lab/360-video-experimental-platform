#include "stdafx.h"
#include "vws_psnr_block.h"

VWSPSNRBlock::VWSPSNRBlock()
    : m_distortion(0.0)
{

}

VWSPSNRBlock::VWSPSNRBlock(const cv::Rect& rect)
    : m_rect(rect)
    , m_distortion(0.0)
{

}

VWSPSNRBlock::~VWSPSNRBlock()
{

}

const cv::Rect& VWSPSNRBlock::GetRect() const
{
    return m_rect;
}

double VWSPSNRBlock::Compute(const cv::Mat& diffMap, const cv::Mat& weightMap)
{
    cv::Mat blockDiffMap(diffMap, m_rect);
    cv::Mat blockWeightMap(weightMap, m_rect);

    cv::Mat blockWeightedDiff = blockDiffMap.mul(blockWeightMap);

    double WMSE = cv::sum(blockWeightedDiff)[0] / cv::sum(blockWeightMap)[0];
    double WSPSNR = 10 * log10(255 * 255 / (WMSE + DBL_EPSILON));

    m_distortion = WSPSNR;

    return m_distortion;
}

double VWSPSNRBlock::GetDistortion() const
{
    return m_distortion;
}
