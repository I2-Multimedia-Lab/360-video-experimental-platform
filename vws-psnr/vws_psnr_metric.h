#pragma once

#include "vws_psnr_config.h"
#include "vws_psnr_tube.h"
#include "vws_psnr_frame.h"

class VWSPSNRMetric
{
public:
    VWSPSNRMetric();
    ~VWSPSNRMetric();

    bool LoadConfig(const VWSPSNRConfig& config);
    bool Init();
    bool Run();
    void Output();

private:
    void Cleanup();

    bool ReadFrameFromSrc(cv::Mat& frame);
    bool ReadFrameFromDst(cv::Mat& frame);

private:
    VWSPSNRConfig m_config;
    FILE* m_fpSrc;
    FILE* m_fpDst;
    cv::VideoCapture m_vcDst;
    int m_yuvSize;
    int m_ySize;
    int m_numFrames;
    bool m_dstNotYuv;
    uint8_t* m_buffer;
    double m_globalPSNR;
};

