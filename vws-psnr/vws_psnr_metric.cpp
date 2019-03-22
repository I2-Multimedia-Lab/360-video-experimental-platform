#include "stdafx.h"
#include "vws_psnr_metric.h"


VWSPSNRMetric::VWSPSNRMetric()
    : m_fpt(NULL)
    , m_fpr(NULL)
    , m_yuvSize(0)
    , m_ySize(0)
    , m_numFrames(0)
{
}


VWSPSNRMetric::~VWSPSNRMetric()
{
    Cleanup();
}

bool VWSPSNRMetric::LoadConfig(const VWSPSNRConfig& config)
{
    if (!config.IsValid())
        return false;

    m_config = config;

    return true;
}

bool VWSPSNRMetric::Init()
{
    Cleanup();

    m_fpt = fopen(m_config.m_tFilename.c_str(), "rb");
    if (m_fpt == NULL)
        return false;

    m_fpr = fopen(m_config.m_rFilename.c_str(), "rb");
    if (m_fpr == NULL)
        return false;

    _fseeki64(m_fpt, 0L, SEEK_END);
    int64_t tFileSize = _ftelli64(m_fpt);
    _fseeki64(m_fpr, 0L, SEEK_END);
    int64_t rFileSize = _ftelli64(m_fpr);
    CV_Assert(tFileSize == rFileSize);

    m_yuvSize = m_config.m_width * m_config.m_height * 3 / 2;
    m_ySize = m_config.m_width * m_config.m_height;
    int numFrames = (int)(tFileSize / m_yuvSize);

    return true;
}

void VWSPSNRMetric::Cleanup()
{
    if (m_fpt != NULL) {
        fclose(m_fpt);
        m_fpt = NULL;
    }
    
    if (m_fpr != NULL) {
        fclose(m_fpr);
        m_fpr = NULL;
    }

    m_yuvSize = m_ySize = 0;
    m_numFrames = 0;
}

bool VWSPSNRMetric::Run()
{


    return true;
}
