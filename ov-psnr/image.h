#pragma once

class Image
{
public:
    Image();
    ~Image();

    bool Create(uint8_t* buffer, int size, int width, int height, int format);
    int Width() const;
    int Height() const;
    int Format() const;
    const cv::Mat& Data() const;

private:
    cv::Mat m_data;
    int m_width;
    int m_height;
    int m_format;
};
