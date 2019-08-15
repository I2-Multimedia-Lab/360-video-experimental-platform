#pragma once

enum MappingFormat
{
    MF_UNKNOWN = 0,
    MF_RECT,
    MF_CUBE,
    // TODO: support more
};

enum InterpolationFilter
{
    IF_NEAREST = 0,
};

class Mapper
{
public:
    Mapper();
    ~Mapper();

public:
    static int StringToFormat(const std::string& format);

    void SphPointFromImg(const Image& img, int format, const Vec2f& sphPoints, Vec1b& pixels);

private:
    void SphToCart(const cv::Point2f& in, cv::Point3f& out);  // Spherical coordinates to Cartesian coordinates
    void SphToRect(const Image& img, const cv::Point3f& in, cv::Point2f& out);

    unsigned char IFilterNearest(const Image& img, const cv::Point2f& in);

private:
    float Clamp(float v, float low, float high);

private:
    int m_ifilter;
};
