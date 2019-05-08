#include "stdafx.h"
#include "vws_psnr_frame.h"

VWSPSNRFrame::VWSPSNRFrame()
    : m_tubeRows(0)
    , m_tubeCols(0)
    , m_distortion(0.0)
    , m_psnr(0.0)
{

}

VWSPSNRFrame::~VWSPSNRFrame()
{
    Cleanup();
}

void VWSPSNRFrame::Cleanup()
{
    m_tubeRows = m_tubeCols = 0;
    m_tubeMap.clear();
    m_distortion = 0.0;
    m_psnr = 0.0;
}

void VWSPSNRFrame::InitTubeMap(int rows, int cols)
{
    Cleanup();

    m_tubeRows = rows;
    m_tubeCols = cols;
    int newSize = m_tubeRows * m_tubeCols;
    if (m_tubeMap.size() != newSize) {
        m_tubeMap.resize(newSize);
        m_tubeMap.shrink_to_fit();
    }
}

VWSPSNRTube& VWSPSNRFrame::GetTube(int x, int y)
{
    int index = y * m_tubeCols + x;
    assert(index < m_tubeRows * m_tubeCols);

    return m_tubeMap.at(index);
}

double VWSPSNRFrame::Pool()
{
    m_distortion = 0.0;
    for (int j = 0; j < m_tubeRows; j++) {
        for (int i = 0; i < m_tubeCols; i++) {
            VWSPSNRTube& tube = GetTube(i, j);

            double d = tube.GetDistortion();
            d = pow(d, 2.0);
            m_distortion += d;
        }
    }
    m_distortion /= m_tubeMap.size();
    m_distortion = sqrt(m_distortion);

    m_psnr = 10 * log10(255 * 255 / (m_distortion + DBL_EPSILON));
    return m_psnr;
}

double VWSPSNRFrame::GetDistortion() const
{
    return m_distortion;
}

double VWSPSNRFrame::GetPSNR() const
{
    return m_psnr;
}
