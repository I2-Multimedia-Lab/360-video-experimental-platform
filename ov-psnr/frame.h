#pragma once

#include "tube.h"
#include "common.h"

class Frame
{
public:
    Frame();
    ~Frame();

    void ConstructTubes(const Segment& segment, int fps);
    void Calculate(const Segment& segment);
    double GetDistortion() const;
    double GetPSNR() const;

private:
    Tube& GetTube(int k, int l);

private:
    Vector<Tube> m_tubes;
    int m_tubeRows;
    int m_tubeCols;
    double m_distortion;
    double m_psnr;
};
