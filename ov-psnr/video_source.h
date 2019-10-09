#pragma once

#include "image.h"

class VideoSource
{
public:
    VideoSource();
    ~VideoSource();

    bool Init(const String& filePath, int width, int height, int format);
    void Cleanup();

    int Format() const;
    int FrameCount() const;
    int Width() const;
    int Height() const;
    bool ReadNextFrame(Image& img);  // only y component

private:
    FILE* m_fp;
    int m_width;
    int m_height;
    int m_format;
    int m_numFrames;
    uint8_t* m_buffer;
};
