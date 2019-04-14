#pragma once

class VWSPSNRBlock
{
public:
    VWSPSNRBlock();
    VWSPSNRBlock(const cv::Rect& rect);
    ~VWSPSNRBlock();

    static const int BLOCK_SIZE = 16;

public:
    const cv::Rect& GetRect() const;
    double Compute(const cv::Mat& diffMap, const cv::Mat& weightMap);
    double GetDistortion() const;

private:
    cv::Rect m_rect;
    double m_distortion;
};

