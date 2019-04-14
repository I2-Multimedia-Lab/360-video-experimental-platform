#include "stdafx.h"
#include "vws_psnr_segment.h"

VWSPSNRSegment::VWSPSNRSegment(int width, int height, int fps)
    : m_frameWidth(width)
    , m_frameHeight(height)
{
    m_len = fps * TEMPORAL_SEGMENT_DURATION / 1000;
    m_interval = 1000 / fps;
}

VWSPSNRSegment::~VWSPSNRSegment()
{

}

void VWSPSNRSegment::PushSrc(const cv::Mat& frame)
{
    m_srcQueue.push_back(frame);
    if ((int)m_srcQueue.size() > m_len)
        m_srcQueue.pop_front();
}

void VWSPSNRSegment::PushDst(const cv::Mat& frame)
{
    m_dstQueue.push_back(frame);
    if ((int)m_dstQueue.size() > m_len)
        m_dstQueue.pop_front();
}

void VWSPSNRSegment::PushDiffMap(const cv::Mat& diffMap)
{
    m_diffMapQueue.push_back(diffMap);
    if ((int)m_diffMapQueue.size() > m_len)
        m_diffMapQueue.pop_front();
}

void VWSPSNRSegment::Process()
{
    // compute distortion map of the last frame in queue
    cv::Mat srcFrame = m_srcQueue.back();
    cv::Mat dstFrame = m_dstQueue.back();

    cv::Mat diffMap;
    cv::absdiff(srcFrame, dstFrame, diffMap);
    diffMap.convertTo(diffMap, CV_64FC1);
    cv::pow(diffMap, 2, diffMap);
    PushDiffMap(diffMap);

    // generate weight map if not exists
    if (m_weightMap.empty())
        GenerateWeightMap(m_weightMap);

    // divide tubes (blocks of last frame)
    int m = m_frameHeight / VWSPSNRBlock::BLOCK_SIZE; // vertical
    int n = m_frameWidth / VWSPSNRBlock::BLOCK_SIZE; // horizontal
    m_frame.InitTubeMap(m, n);

    // construct tubes
    for (int j = 0; j < m; j++) { // row
        for (int i = 0; i < n; i++) { // col
            VWSPSNRTube& tube = m_frame.GetTube(i, j);

            int x = i * VWSPSNRBlock::BLOCK_SIZE;
            int y = j * VWSPSNRBlock::BLOCK_SIZE;
            cv::Rect rcBlock(x, y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE);
            tube.Create(m_srcQueue, rcBlock);
            tube.Compute(m_diffMapQueue, m_weightMap, m_interval);
        }
    }

    // spatial pooling
    m_frame.Pool();
}

const VWSPSNRFrame& VWSPSNRSegment::GetFrame() const
{
    return m_frame;
}

void VWSPSNRSegment::GenerateWeightMap(cv::Mat& weightMap)
{
    weightMap = cv::Mat::ones(m_frameHeight, m_frameWidth, CV_64FC1);

    // equirectangular weight from WS-PSNR
    for (int j = 0; j < weightMap.rows; j++) {
        double weight = cos((j - (m_frameHeight / 2 - 0.5)) * M_PI / m_frameHeight);
        weightMap.row(j) = weightMap.row(j) * weight;
    }
}
