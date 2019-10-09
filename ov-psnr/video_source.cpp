#include "stdafx.h"
#include "video_source.h"
#include "common.h"

VideoSource::VideoSource()
    : m_fp(nullptr)
    , m_width(0)
    , m_height(0)
    , m_format(0)
    , m_buffer(nullptr)
{

}

VideoSource::~VideoSource()
{
    Cleanup();
}

bool VideoSource::Init(const String& filePath, int width, int height, int format)
{
    if (filePath.substr(filePath.length() - 4) != ".yuv")  // TODO: support non-yuv video
        return false;

    Cleanup();

    m_fp = fopen(filePath.c_str(), "rb");
    if (m_fp == nullptr)
        return false;

    m_width = width;
    m_height = height;
    m_format = format;

    int64_t yuvSize = m_width * m_height * 3 / 2;
    m_buffer = new uint8_t[yuvSize];

    _fseeki64(m_fp, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(m_fp);
    m_numFrames = (int)(fileSize / yuvSize);
    _fseeki64(m_fp, 0L, SEEK_SET);

    return true;
}

void VideoSource::Cleanup()
{
    if (m_fp != nullptr) {
        fclose(m_fp);
        m_fp = nullptr;
    }

    if (m_buffer != nullptr) {
        delete[] m_buffer;
        m_buffer = nullptr;
    }
}

int VideoSource::Format() const
{
    return m_format;
}

int VideoSource::FrameCount() const
{
    return m_numFrames;
}

int VideoSource::Width() const
{
    return m_width;
}

int VideoSource::Height() const
{
    return m_height;
}

bool VideoSource::ReadNextFrame(Image& img)
{
    assert(m_format != GT_UNKNOWN);

    int yuvSize = m_width * m_height * 3 / 2;
    int ySize = m_width * m_height;

    if (fread(m_buffer, yuvSize, 1, m_fp) != 1)
        return false;

    if (!img.Create(m_buffer, ySize, m_width, m_height, m_format))
        return false;

    return true;
}
