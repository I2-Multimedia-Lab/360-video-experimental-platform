#pragma once

#include "vws_psnr_tube.h"

class VWSPSNRFrame
{
public:
    VWSPSNRFrame();
    ~VWSPSNRFrame();

public:
    void Cleanup();
    void InitTubeMap(int rows, int cols);
    VWSPSNRTube& GetTube(int x, int y);
    double Pool();
    double GetDistortion() const;
    double GetPSNR() const;

private:
    int m_tubeRows, m_tubeCols;
    std::vector<VWSPSNRTube> m_tubeMap;
    double m_distortion;
    double m_psnr;
};
