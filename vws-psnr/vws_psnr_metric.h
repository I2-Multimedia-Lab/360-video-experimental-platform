#pragma once

#include "vws_psnr_config.h"

class VWSPSNRMetric
{
public:
    VWSPSNRMetric();
    ~VWSPSNRMetric();

    bool LoadConfig(const VWSPSNRConfig& config);
    bool Init();
    bool Run();

private:
    void Cleanup();

private:
    VWSPSNRConfig m_config;
    FILE* m_fpt;
    FILE* m_fpr;
    int m_yuvSize;
    int m_ySize;
    int m_numFrames;
};

