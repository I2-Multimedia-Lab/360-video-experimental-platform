#include "stdafx.h"
#include "vws_psnr_metric.h"
#include "vws_psnr_segment.h"

VWSPSNRMetric::VWSPSNRMetric()
    : m_fpSrc(NULL)
    , m_fpDst(NULL)
    , m_yuvSize(0)
    , m_ySize(0)
    , m_numFrames(0)
    , m_dstNotYuv(false)
    , m_buffer(NULL)
    , m_globalDistortion(0.0)
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

    const std::string& srcFilename = m_config.m_srcFilename;
    const std::string& dstFilename = m_config.m_dstFilename;

    assert(srcFilename.substr(srcFilename.length() - 4) == ".yuv");

    m_fpSrc = fopen(srcFilename.c_str(), "rb");
    if (m_fpSrc == NULL)
        return false;

    if (dstFilename.substr(dstFilename.length() - 4) != ".yuv") {
        m_dstNotYuv = true;
        if (!m_vcDst.open(dstFilename))
            return false;
    }
    else {
        m_fpDst = fopen(dstFilename.c_str(), "rb");
        if (m_fpDst == NULL)
            return false;
    }

    _fseeki64(m_fpSrc, 0L, SEEK_END);
    int64_t srcFileSize = _ftelli64(m_fpSrc);

    m_yuvSize = m_config.m_width * m_config.m_height * 3 / 2;
    m_ySize = m_config.m_width * m_config.m_height;
    m_numFrames = (int)(srcFileSize / m_yuvSize);
    _fseeki64(m_fpSrc, 0L, SEEK_SET);

    m_buffer = new uint8_t[m_yuvSize];

    m_frameDistortions.reserve(m_numFrames);

    return true;
}

void VWSPSNRMetric::Cleanup()
{
    if (m_fpSrc != NULL) {
        fclose(m_fpSrc);
        m_fpSrc = NULL;
    }
    
    if (m_fpDst != NULL) {
        fclose(m_fpDst);
        m_fpDst = NULL;
    }

    m_vcDst.release();

    m_yuvSize = m_ySize = 0;
    m_numFrames = 0;
    m_dstNotYuv = false;

    if (m_buffer != NULL) {
        delete[] m_buffer;
        m_buffer = NULL;
    }

    m_frameDistortions.clear();

    m_globalDistortion = 0.0;
}

bool VWSPSNRMetric::Run()
{
    VWSPSNRSegment segment(m_config.m_width, m_config.m_height, m_config.m_fps);  // some kind of cache
    cv::Mat frameSrc;
    cv::Mat frameDst;
    bool r = false;
    for (int i = 0; i < m_numFrames; i++) {
        printf("Frame %d ", i);
        clock_t start = clock();

        r = ReadFrameFromSrc(frameSrc);
        assert(r);

        r = ReadFrameFromDst(frameDst);
        assert(r);

        segment.PushSrc(frameSrc);
        segment.PushDst(frameDst);
        
        segment.Process();

        double frameDistortion = segment.GetFrame().GetDistortion();
        m_frameDistortions.push_back(frameDistortion);

        clock_t end = clock();
        printf(" %.2lf  %.1lfs\n", frameDistortion, (double)(end - start) / CLOCKS_PER_SEC);
    }

    m_globalDistortion = 0.0;
    for (int i = 0; i < m_numFrames; i++) {
        double frameDistortion = m_frameDistortions[i];
        m_globalDistortion += frameDistortion;
    }
    m_globalDistortion /= m_numFrames;

    return true;
}

bool VWSPSNRMetric::ReadFrameFromSrc(cv::Mat& frame)
{
    assert(m_fpSrc != NULL);

    if (fread(m_buffer, m_yuvSize, 1, m_fpSrc) != 1)
        return false;

    frame.create(m_config.m_height, m_config.m_width, CV_8UC1);
    memcpy(frame.data, m_buffer, m_ySize);

    return true;
}

bool VWSPSNRMetric::ReadFrameFromDst(cv::Mat& frame)
{
    if (m_dstNotYuv) {
        assert(m_vcDst.isOpened());

        if (!m_vcDst.read(frame))
            return false;

        // extract Y channel
        cv::extractChannel(frame, frame, 0);
    }
    else {
        assert(m_fpDst != NULL);

        if (fread(m_buffer, m_yuvSize, 1, m_fpDst) != 1)
            return false;

        frame.create(m_config.m_height, m_config.m_width, CV_8UC1);
        memcpy(frame.data, m_buffer, m_ySize);
    }
        
    return true;
}

void VWSPSNRMetric::Output()
{
    printf("The global score is %.2lf \n", m_globalDistortion);
}
