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
    IF_LANCZOS,
};

class Mapper
{
public:
    Mapper();
    ~Mapper();

public:
    static int StringToFormat(const std::string& format);
    void SetIFilter(int ifilter);

    void SphPointFromImg(const Image& img, int format, const Vec2f& sphPoints, Vec1d& pixels);

private:
    void SphToCart(const cv::Point2f& in, cv::Point3f& out);  // Spherical coordinates to Cartesian coordinates
    void CartToRect(const Image& img, const cv::Point3f& in, cv::Point2f& out);
    void CartToCube(const Image& img, const cv::Point3f& in, cv::Point2f& out, int& faceIdx);
    std::vector<Image> ExtractCubeFace(const Image& cmp);

    unsigned char IFilterNearest(const Image& img, const cv::Point2f& in);
    void InitLanczosCoef();
    double IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const;

private:
    float Clamp(float v, float low, float high);

private:
    int m_ifilter;
    std::vector<double> m_lanczosCoef;
};
