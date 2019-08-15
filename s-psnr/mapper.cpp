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
    float lat = in.x * PI / 180.0f;
    float lon = in.y * PI / 180.0f;

    /* 
        the cartesian coodinate system:
                    y
                    |
                    |_ _ _ x
                   / o
                z /
    */

    out.x = sinf(lon) * cosf(lat);
    out.y = sinf(lat);
    out.z = -cosf(lon) * cosf(lat);
}

void Mapper::SphToRect(const Image& img, const cv::Point3f& in, cv::Point2f& out)
{
    int w = img.cols;
    int h = img.rows;

    /* 
        --------------------
       |               /\   |       theta: [-pi,pi]
       y  < theta >    phi  |   
       |               \/   |       phi: [0, pi] 
        --------- x --------
    */

    float theta = atan2f(in.x, -in.z);
    float phi = acosf(in.y);

    out.x = w * (0.5f + theta / TWOPI);
    out.y = h * (phi / PI);
}

unsigned char Mapper::IFilterNearest(const Image& img, const cv::Point2f& in)
{
    int w = img.cols;
    int h = img.rows;
    
    float fx = Clamp(in.x - 0.5f, 0.0f, w - 1.0f);
    float fy = Clamp(in.y - 0.5f, 0.0f, h - 1.0f);

    int lx = lrintf(fx);
    int ly = lrintf(fy);

    return img.at<uchar>(ly, lx);
}

float Mapper::Clamp(float v, float low, float high)
{
    if (v < low) return low;
    else if (v > high) return high;
    else return v;
}
