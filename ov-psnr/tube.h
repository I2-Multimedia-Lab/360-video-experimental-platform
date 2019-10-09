#pragma once

#include "block.h"
#include "image.h"
#include "common.h"

class Tube
{
public:
    Tube();
    ~Tube();

    void Create(const Deque<std::pair<Image, Image>>& frameQueue, Point startPos, int interval);
    void Calculate(const Segment& segment);
    double GetDistortion() const;

private:
    bool MatchBlock(const Image& src, const Image& dst, const Point& from, Point& to);
    cv::Mat1d Match(const cv::Mat& src, const cv::Mat& dst, const cv::Point& origin, const cv::Point& search, int step = 1, bool skipNOC = false);
    double MAD(cv::InputArray s1, cv::InputArray s2);

    double TemporalDistortionCoefficient(double mg, int sc);

private:
    Deque<Block> m_blocks;
    int m_interval;
    double m_distortion;
};
