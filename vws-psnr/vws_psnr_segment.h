#pragma once

#include "vws_psnr_frame.h"

class VWSPSNRSegment
{
public:
    VWSPSNRSegment(int width, int height, int fps);
    ~VWSPSNRSegment();

    static const int TEMPORAL_SEGMENT_DURATION = 400;  // 400ms

public:
    void PushSrc(const cv::Mat& frame);
    void PushDst(const cv::Mat& frame);

    void Process();

    const VWSPSNRFrame& GetFrame() const;

private:
    void PushDiffMap(const cv::Mat& diffMap);
    void GenerateWeightMap(cv::Mat& weightMap);

private:
    int m_len;
    int m_frameWidth;
    int m_frameHeight;
    int m_interval;
    std::deque<cv::Mat> m_srcQueue;
    std::deque<cv::Mat> m_dstQueue;
    std::deque<cv::Mat> m_diffMapQueue;  // square difference map between src and dst
    cv::Mat m_weightMap;
    VWSPSNRFrame m_frame;
};
