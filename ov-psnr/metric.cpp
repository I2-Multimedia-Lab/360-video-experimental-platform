#include "stdafx.h"
#include "metric.h"
#include "frame.h"

#define DEFAULT_FPS 25
#define DEFAULT_CAP 10
#define TEMPORAL_HORIZON 400  // ms

Metric::Metric()
    : m_metric(MT_UNKNOWN)
    , m_fps(DEFAULT_FPS)
    , m_segmentCapacity(DEFAULT_CAP)
    , m_globalPSNR(0.0)
{

}

Metric::~Metric()
{

}

bool Metric::Init(const Config& cfg)
{
    if (!cfg.IsValid())
        return false;

    if (!m_src.Init(cfg.m_srcFilename, cfg.m_srcWidth, cfg.m_srcHeight, cfg.m_srcFormat)) {
        printf("Source video load failed!\n");
        return false;
    }
        
    if (!m_dst.Init(cfg.m_dstFilename, cfg.m_dstWidth, cfg.m_dstHeight, cfg.m_dstFormat)) {
        printf("Destination video load failed!\n");
        return false;
    }
        
    if (m_src.FrameCount() != m_dst.FrameCount()) {
        printf("Source video and destination video must hava same frame count.\n");
        return false;
    }

    m_metric = cfg.m_metric;
    m_fps = cfg.m_fps;
    m_segmentCapacity = m_fps * TEMPORAL_HORIZON / 1000;

    // TODO: support more source format
    if (m_src.Format() != GT_EQUIRECT) {
        printf("Source video format only supports ERP.\n");
        return false;
    }

    if (cfg.m_metric == MT_SPSNR_NN) {
        printf("[S-PSNR]Read Spheric points...\n");
        if (!m_spsnr.ReadSphPoint(cfg.m_sphPointFile)) {
            printf("Spheric points file read failed.\n");
            return false;
        }
        printf("[S-PSNR]Generate Rect-To-Sph Map...");
        clock_t start = clock();
        m_spsnr.GenerateR2SMap(m_src.Width(), m_src.Height());  // take times
        printf("%.1lfs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    }
    else if (cfg.m_metric == MT_CPPPSNR) {
        m_cpppsnr.Init(m_src.Width(), m_src.Height());
    }

    return true;
}

void Metric::Compare()
{
    double globalDistortion = 0.0;

    int numFrames = m_src.FrameCount();
    for (int i = 0; i < numFrames; i++) {
        printf("Frame %d: ", i);
        Frame curFrame;

        // read image
        Image srcImage;
        m_src.ReadNextFrame(srcImage);
        Image dstImage;
        m_dst.ReadNextFrame(dstImage);

        PushFrame(std::move(srcImage), std::move(dstImage));

        // construct tubes
        clock_t start = clock();
        curFrame.ConstructTubes(m_segment, m_fps);
        printf("%.1lfs, ", (double)(clock() - start) / CLOCKS_PER_SEC);

        // calculate distortion
        start = clock();
        CompareLastFrame();
        curFrame.Calculate(m_segment);
        printf("%.1lfs, ", (double)(clock() - start) / CLOCKS_PER_SEC);
        printf("%.2lf", curFrame.GetPSNR());

        printf("\n");

        globalDistortion += curFrame.GetDistortion();
    }

    globalDistortion /= numFrames;

    m_globalPSNR = 10 * log10(255 * 255 / (globalDistortion + DBL_EPSILON));
}

void Metric::Output()
{
    printf("The global PSNR is %.2lf\n", m_globalPSNR);
}

void Metric::PushFrame(Image&& src, Image&& dst)
{
    Deque<std::pair<Image, Image>>& frameQueue = m_segment.m_frameQueue;
    frameQueue.emplace_back(std::make_pair(src, dst));
    if ((int)frameQueue.size() > m_segmentCapacity)
        frameQueue.pop_front();
}

void Metric::CompareLastFrame()
{
    Deque<std::pair<Image, Image>>& frameQueue = m_segment.m_frameQueue;
    const Image& lastSrc = frameQueue.back().first;
    const Image& lastDst = frameQueue.back().second;

    DistortionMap* dMap = nullptr;
    switch (m_metric)
    {
    case MT_PSNR:
        dMap = m_psnr.Calculate(lastSrc, lastDst);
        break;
    case MT_WSPSNR:
        dMap = m_wspsnr.Calculate(lastSrc, lastDst);
        break;
    case MT_SPSNR_NN:
        dMap = m_spsnr.Calculate(lastSrc, lastDst);
        break;
    case MT_CPPPSNR:
        dMap = m_cpppsnr.Calculate(lastSrc, lastDst);
        break;
    }
    assert(dMap != nullptr);

    Deque<DistortionMap*>& distortionMapQueue = m_segment.m_distortionMapQueue;
    distortionMapQueue.emplace_back(dMap);
    if ((int)distortionMapQueue.size() > m_segmentCapacity) {
        dMap = distortionMapQueue.front();
        distortionMapQueue.pop_front();
        delete dMap;
    }
}
