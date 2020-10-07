#pragma once

#include "config.h"
#include "video_source.h"
#include "common.h"
#include "psnr_distortion.h"
#include "wspsnr_distortion.h"
#include "spsnr_distortion.h"
#include "cpppsnr_distortion.h"

class Metric
{
public:
    Metric();
    ~Metric();

public:
    bool Init(const Config& cfg);
    void Compare();
    void Output();

private:
    void PushFrame(Image&& src, Image&& dst);
    void CompareLastFrame();

private:
    VideoSource m_src;
    VideoSource m_dst;
    int m_metric;
    int m_fps;
    Segment m_segment;
    int m_segmentCapacity;
    PSNRDistortion m_psnr;
    WSPSNRDistortion m_wspsnr;
    SPSNRDistortion m_spsnr;
    CPPPSNRDistortion m_cpppsnr;
    double m_globalPSNR;
    double m_averageDuration;
};
