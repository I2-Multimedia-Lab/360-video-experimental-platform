#include "stdafx.h"
#include "image.h"


Image::Image()
    : m_width(0)
    , m_height(0)
{

}

Image::~Image()
{

}

bool Image::Create(uint8_t* buffer, int size, int width, int height, int format)
{
    m_data.create(height, width, CV_8UC1);
    memcpy(m_data.data, buffer, size);

    m_width = width;
    m_height = height;
    m_format = format;

    return true;
}

int Image::Width() const
{
    return m_width;
}

int Image::Height() const
{
    return m_height;
}

int Image::Format() const
{
    return m_format;
}

const cv::Mat& Image::Data() const
{
    return m_data;
}
