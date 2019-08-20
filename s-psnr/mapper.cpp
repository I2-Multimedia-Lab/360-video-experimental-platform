#include "stdafx.h"
#include "mapper.h"

#define PI 3.1415927f
#define TWOPI 6.2831853f

Mapper::Mapper()
    : m_ifilter(IF_NEAREST)
{

}

Mapper::~Mapper()
{

}

int Mapper::StringToFormat(const std::string& format)
{
    int r = MF_UNKNOWN;

    if (format.compare("rect") == 0)
        r = MF_RECT;
    else if (format.compare("cube"))
        r = MF_CUBE;

    return r;
}

void Mapper::SphPointFromImg(const Image& img, int format, const Vec2f& sphPoints, Vec1b& pixels)
{
    pixels.resize(sphPoints.size());

    for (size_t i = 0; i < sphPoints.size(); i++) {
        cv::Point3f cartPoint;
        const cv::Point2f& sphPoint = sphPoints[i];
        SphToCart(sphPoint, cartPoint);  // (r, phi, theta) -> (x, y, z)

        cv::Point2f planePoint;
        switch (format) {
        case MF_RECT:
            SphToRect(img, cartPoint, planePoint);
            break;
        case MF_CUBE:
            break;
        default:
            assert(false);
            break;
        }

        switch (m_ifilter) {
        case IF_NEAREST:
            pixels[i] = IFilterNearest(img, planePoint);
            break;
        default:
            assert(false);
            break;
        }
    }
}

void Mapper::SphToCart(const cv::Point2f& in, cv::Point3f& out)
{
    float lon = in.y * PI / 180.0f;  // phi
    float lat = in.x * PI / 180.0f;  // theta

    out.x = cosf(lat) * cosf(lon);
    out.y = sinf(lat);
    out.z = -cosf(lat) * sinf(lon);
}

void Mapper::SphToRect(const Image& img, const cv::Point3f& in, cv::Point2f& out)
{
    int w = img.cols;
    int h = img.rows;

    float phi = atan2f(-in.z, in.x);
    float theta = asinf(in.y);

    float u = phi / TWOPI + 0.5f;
    float v = 0.5f - theta / PI;

    float m = w * u - 0.5f;
    float n = h * v - 0.5f;

    out.x = Clamp(m - 0.5f, 0.0f, w - 1.0f);
    out.y = Clamp(n - 0.5f, 0.0f, h - 1.0f);
}

unsigned char Mapper::IFilterNearest(const Image& img, const cv::Point2f& in)
{
    int w = img.cols;
    int h = img.rows;
    
    int lx = lrintf(in.x);
    int ly = lrintf(in.y);

    return img.at<uchar>(ly, lx);
}

float Mapper::Clamp(float v, float low, float high)
{
    if (v < low) return low;
    else if (v > high) return high;
    else return v;
}
