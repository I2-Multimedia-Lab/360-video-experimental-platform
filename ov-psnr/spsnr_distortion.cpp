#include "stdafx.h"
#include "spsnr_distortion.h"

#define M_2PI (6.2831853f)  /* pi*2 */

#define DEG2RAD(deg) ((deg) * (float)M_PI / 180.0f)
#define RAD2DEG(rad) ((rad) * 180.0f / (float)M_PI)

SPSNRDistortionMap::SPSNRDistortionMap()
{

}

SPSNRDistortionMap::~SPSNRDistortionMap()
{

}

double SPSNRDistortionMap::GetBlcokDistortion(const Rect& rc) const
{
    cv::Mat blockDiffMap(m_diffMap, rc);

    double MSE = (double)cv::sum(blockDiffMap)[0] / blockDiffMap.total();

    return MSE;
}

SPSNRDistortion::SPSNRDistortion()
{

}

SPSNRDistortion::~SPSNRDistortion()
{

}

SPSNRDistortionMap* SPSNRDistortion::Calculate(const Image& src, const Image& dst)
{
    assert(!m_cartPoints.empty());

    Vector<double> diffVals;
    for (auto& cartPoint : m_cartPoints) {
        cv::Point2f srcPoint;
        CartToRect(cartPoint, src.Width(), src.Height(), srcPoint);
        double srcVal = IFilterNearest(src.Data(), srcPoint);

        cv::Point2f dstPoint;
        CartToRect(cartPoint, dst.Width(), dst.Height(), dstPoint);
        double dstVal = IFilterNearest(dst.Data(), dstPoint);

        double diff = pow(dstVal - srcVal, 2);

        diffVals.push_back(diff);
    }

    GenerateR2SMap(src.Width(), src.Height());  // supposed that src is erp format!

    SPSNRDistortionMap* distortionMap = new SPSNRDistortionMap;
    cv::Mat& diffMap = distortionMap->m_diffMap;
    diffMap.create(src.Height(), src.Width(), CV_64FC1);

    for (int i = 0; i< diffMap.rows; i++) {
        for (int j = 0; j< diffMap.cols; j++) {
            int index = m_r2sMap.at<unsigned short>(i, j);
            assert(index > 0 && index < diffVals.size());
            diffMap.at<double>(i, j) = diffVals[index];
        }
    }

    return distortionMap;
}

bool SPSNRDistortion::ReadSphPoint(const String& sphPointFile)
{
    int numPoints = 0;
    char tmp[32];
    if (sscanf(sphPointFile.c_str(), "%[^_]_%d.txt", tmp, &numPoints) != 2)
        return false;

    FILE* fp = fopen(sphPointFile.c_str(), "r");
    if (fp == nullptr)
        return false;

    m_cartPoints.resize(numPoints);

    bool result = true;
    for (int i = 0; i < numPoints; i++) {
        float lat, lon;
        if (fscanf(fp, "%f %f", &lat, &lon) != 2) {
            result = false;
            break;
        }

        cv::Point3f cart;
        SphToCart(cv::Point2f(lat, lon), cart);

        m_cartPoints[i] = std::move(cart);
    }

    fclose(fp);

    cv::flann::KDTreeIndexParams kdtreeIndex(1);
    cv::Mat features = cv::Mat(m_cartPoints).reshape(1);
    m_kdtree.build(features, kdtreeIndex);

    return result;
}

void SPSNRDistortion::GenerateR2SMap(int width, int height)
{
    if (!m_r2sMap.empty())
        return;

    m_r2sMap.create(height, width, CV_16UC1);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            cv::Point2f rectPoint((float)j, (float)i);
            cv::Point3f cartPoint;
            RectToCart(rectPoint, width, height, cartPoint);

            Vector<float> query{ cartPoint.x, cartPoint.y, cartPoint.z };
            Vector<int> index;
            Vector<float> dist;
            int knn = 1;
            m_kdtree.knnSearch(query, index, dist, knn, cv::flann::SearchParams());

            assert(index.size() > 0 && index[0] < m_cartPoints.size());
            m_r2sMap.at<unsigned short>(i, j) = index[0];
        }
    }
}

void SPSNRDistortion::SphToCart(const cv::Point2f& in, cv::Point3f& out) const
{
    float lat = DEG2RAD(in.x);
    float lon = DEG2RAD(in.y);

    out.x = sinf(lon) * cosf(lat);
    out.y = sinf(lat);
    out.z = -cosf(lon) * cosf(lat);
}

void SPSNRDistortion::CartToSph(const cv::Point3f& in, cv::Point2f& out) const
{
    out.x = (float)RAD2DEG(acos(in.z));
    if (out.x > 180) out.x = 180;

    if (in.x < 0)
    {
        out.y = 180.0f - (float)RAD2DEG(-atan(in.y / in.x));
    }
    else if (in.x == 0)
    {
        if (in.y < 0)
        {
            out.y = 270;
        }
        else if (in.y == 0)
        {
            out.y = (in.z < 0 ? 270.0f : 90.0f);
        }
        else
        {
            out.y = 90;
        }
    }
    else /* x>0 */
    {
        out.y = (float)RAD2DEG(atan(in.y / in.x));
        if (out.y < 0) out.y += 360;
    }

    if (out.y == 0) out.y = 0.01f;
    if (out.y == 360) out.y = 359.99f;
}

void SPSNRDistortion::CartToRect(const cv::Point3f& in, int w, int h, cv::Point2f& out) const
{
    float phi = acosf(in.y);
    float theta = atan2f(in.x, in.z);

    out.x = w * (0.5f + theta / (float)M_2PI);
    out.y = h * (phi / (float)M_PI);
}

void SPSNRDistortion::RectToCart(const cv::Point2f& in, int w, int h, cv::Point3f& out) const
{
    float lat = (float)(M_PI_2 - M_PI * in.y / h);
    float lon = (float)(M_2PI * in.x / w - M_PI);

    out.x = sinf(lon) * cosf(lat);
    out.y = sinf(lat);
    out.z = -cosf(lon) * cosf(lat);
}

double SPSNRDistortion::IFilterNearest(const cv::Mat& img, const cv::Point2f& in) const
{
    int w = img.cols;
    int h = img.rows;

    float x = Clamp(in.x - 0.5f, 0.0f, w - 1.0f);
    float y = Clamp(in.y - 0.5f, 0.0f, h - 1.0f);

    int lx = lrintf(x);
    int ly = lrintf(y);

    return (double)img.at<uchar>(ly, lx);
}

float SPSNRDistortion::Clamp(float v, float low, float high) const
{
    if (v < low) return low;
    else if (v > high) return high;
    else return v;
}
