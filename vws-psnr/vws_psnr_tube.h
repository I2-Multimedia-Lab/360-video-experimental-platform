#pragma once

#include "vws_psnr_block.h"

class VWSPSNRTube
{
public:
    VWSPSNRTube();
    ~VWSPSNRTube();

public:
    void Cleanup();
    bool Create(const std::deque<cv::Mat>& frameQueue, const cv::Rect firstBlockRect);
    double Compute(const std::deque<cv::Mat>& diffMapQueue, const cv::Mat& weightMap, int interval);
    double GetDistortion() const;

private:
    bool MatchBlock(const cv::Mat& c, const cv::Mat& r, cv::Rect& rc);
    cv::Mat1d Match(const cv::Mat& src, const cv::Mat& dst, const cv::Point& origin, const cv::Point& search, int step = 1, bool skipNOC = false);
    double MAD(cv::InputArray s1, cv::InputArray s2);
    double TemporalDistortionCoefficient(double mg, int sc);

private:
    std::deque<VWSPSNRBlock> m_blocks;
    double m_distortion;
};
