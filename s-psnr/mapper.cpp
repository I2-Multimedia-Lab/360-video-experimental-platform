#include "stdafx.h"
#include "mapper.h"

#define M_2PI (6.2831853f)  /* pi*2 */

#define DEG2RAD(deg) ((deg) * (float)M_PI / 180.0f)
#define RAD2DEG(rad) ((rad) * 180.0f / (float)M_PI)

#define LANCZOS_TAB_SIZE        3
#define LANCZOS_FAST_SCALE      100
#define LANCZOS_FAST_MAX_SIZE   (LANCZOS_TAB_SIZE << 1)
#define lanczos_coef(x)         m_lanczosCoef[(int)(fabs(x) * LANCZOS_FAST_SCALE + 0.5)]

Mapper::Mapper()
    : m_ifilter(IF_NEAREST)
{
    InitLanczosCoef();
}

Mapper::~Mapper()
{

}

int Mapper::StringToFormat(const std::string& format)
{
    int r = MF_UNKNOWN;

    if (format.compare("rect") == 0)
        r = MF_RECT;
    else if (format.compare("cube") == 0)
        r = MF_CUBE;

    return r;
}

void Mapper::SetIFilter(int ifilter)
{
    assert(ifilter == IF_NEAREST || ifilter == IF_LANCZOS);
    m_ifilter = ifilter;
}

void Mapper::SphPointFromImg(const Image& img, int format, const Vec2f& sphPoints, Vec1d& pixels)
{
    pixels.resize(sphPoints.size());

    std::vector<Image> cmpFaces;
    if (format == MF_CUBE)
        cmpFaces = std::move(ExtractCubeFace(img));

    for (size_t i = 0; i < sphPoints.size(); i++) {
        cv::Point3f cartPoint;
        const cv::Point2f& sphPoint = sphPoints[i];
        SphToCart(sphPoint, cartPoint);  // (r, phi, theta) -> (x, y, z)

        cv::Point2f planePoint;
        Image face;
        switch (format) {
        case MF_RECT:
            CartToRect(img, cartPoint, planePoint);
            face = img;
            break;
        case MF_CUBE:
            int faceIdx;
            CartToCube(img, cartPoint, planePoint, faceIdx);
            face = cmpFaces[faceIdx];
            break;
        default:
            assert(false);
            break;
        }

        switch (m_ifilter) {
        case IF_NEAREST:
            pixels[i] = (double)IFilterNearest(face, planePoint);
            break;
        case IF_LANCZOS:
            pixels[i] = IFilterLanczos(face, planePoint);
            break;
        default:
            assert(false);
            break;
        }
    }
}

void Mapper::SphToCart(const cv::Point2f& in, cv::Point3f& out)
{
    float lat = DEG2RAD(in.x);
    float lon = DEG2RAD(in.y);

    out.x = cosf(lat) * cosf(lon);
    out.y = sinf(lat);
    out.z = -cosf(lat) * sinf(lon);
}

void Mapper::CartToRect(const Image& img, const cv::Point3f& in, cv::Point2f& out)
{
    int w = img.cols;
    int h = img.rows;

    float lambda = atan2f(-in.z, in.x);
    float phi = asinf(in.y);

    float u = lambda / (float)M_2PI + 0.5f;
    float v = 0.5f - phi / (float)M_PI;

    out.x = w * u - 0.5f;
    out.y = h * v - 0.5f;
}

void Mapper::CartToCube(const Image& img, const cv::Point3f& in, cv::Point2f& out, int& faceIdx)
{
    int w = img.cols;
    int h = img.rows;

    int A = w / 4;
    
    // ref JVET-D0021 
    std::vector<cv::Rect> faceArea ({
        { 2 * A, A, A, A },  // PX
        { 0, A, A, A },      // NX
        { A, 0, A, A },      // PY
        { A, 2 * A, A, A },  // NY
        { A, A, A, A },      // PZ
        { 3 * A, A, A, A }   // NZ
    });

    float aX = fabsf(in.x);
    float aY = fabsf(in.y);
    float aZ = fabsf(in.z);

    float u, v;

    if (aX >= aY && aX >= aZ) {
        if (in.x > 0) {
            faceIdx = 0;
            u = -in.z / aX;
            v = -in.y / aX;
        }
        else {
            faceIdx = 1;
            u = in.z / aX;
            v = -in.y / aX;
        }
    }
    else if (aY >= aX && aY >= aZ) {
        if (in.y > 0) {
            faceIdx = 2;
            u = in.x / aY;
            v = in.z / aY;
        }
        else {
            faceIdx = 3;
            u = in.x / aY;
            v = -in.z / aY;
        }
    }
    else if(aZ >= aX && aZ >= aY) {
        if (in.z > 0) {
            faceIdx = 4;
            u = in.x / aZ;
            v = -in.y / aZ;
        }
        else {
            faceIdx = 5;
            u = -in.x / aZ;
            v = -in.y / aZ;
        }
    }
    else {
        assert(false);
    }

    assert(faceIdx >= 0 && faceIdx <= 5);

    float m = (u + 1.0f) * (A / 2.0f) - 0.5f;
    float n = (v + 1.0f) * (A / 2.0f) - 0.5f;
    assert(m > -1 && m < A + 1);
    assert(n > -1 && n < A + 1);

    out.x = m;
    out.y = n;
}

std::vector<Image> Mapper::ExtractCubeFace(const Image& cmp)
{
    int w = cmp.cols;
    int h = cmp.rows;

    int A = w / 4;

    std::vector<cv::Rect> faceArea({
        { 2 * A, A, A, A },  // PX
        { 0, A, A, A },      // NX
        { A, 0, A, A },      // PY
        { A, 2 * A, A, A },  // NY
        { A, A, A, A },      // PZ
        { 3 * A, A, A, A }   // NZ
    });

    std::vector<Image> faces(6);
    faces[0] = cmp(faceArea[0]);
    faces[1] = cmp(faceArea[1]);
    faces[2] = cmp(faceArea[2]);
    faces[3] = cmp(faceArea[3]);
    faces[4] = cmp(faceArea[4]);
    faces[5] = cmp(faceArea[5]);

    return faces;
}

unsigned char Mapper::IFilterNearest(const Image& img, const cv::Point2f& in)
{
    int w = img.cols;
    int h = img.rows;

    float x = Clamp(in.x - 0.5f, 0.0f, w - 1.0f);
    float y = Clamp(in.y - 0.5f, 0.0f, h - 1.0f);

    int lx = lrintf(x);
    int ly = lrintf(y);

    return img.at<uchar>(ly, lx);
}

float Mapper::Clamp(float v, float low, float high)
{
    if (v < low) return low;
    else if (v > high) return high;
    else return v;
}

static double sinc(double x)
{
    x *= M_PI;
    if (x < 0.01 && x > -0.01)
    {
        double x2 = x * x;
        return 1.0f + x2 * (-1.0 / 6.0 + x2 / 120.0);
    }
    else
    {
        return sin(x) / x;
    }
}

void Mapper::InitLanczosCoef()
{
    if (!m_lanczosCoef.empty())
        return;

    m_lanczosCoef.resize(LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE);

    for (int i = 0; i < LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE; i++) {
        float x = (float)i / LANCZOS_FAST_SCALE;
        m_lanczosCoef[i] = sinc(x) * sinc(x / LANCZOS_TAB_SIZE);
    }
}

double Mapper::IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const
{
    int w = img.cols;
    int h = img.rows;

    double sum = 0.0;
    double res = 0.0;
    for (int j = -LANCZOS_TAB_SIZE; j < LANCZOS_TAB_SIZE; j++) {
        for (int i = -LANCZOS_TAB_SIZE; i < LANCZOS_TAB_SIZE; i++) {
            int idx_x = (int)in.x + i + 1;
            int idx_y = (int)in.y + j + 1;

            if (idx_x >= 0 && idx_y >= 0 && idx_x < w && idx_y < h) {
                double coef = lanczos_coef(in.x - idx_x) * lanczos_coef(in.y - idx_y);
                res += img.at<uchar>(idx_y, idx_x) * coef;
                sum += coef;
            }
        }
    }

    assert(sum != 0.0);
    double val = res / sum;
    return val;
}

