#include "stdafx.h"
#include "spsnr_distortion.h"
#include "common.h"

#define M_2PI (6.2831853f)  /* pi*2 */

#define DEG2RAD(deg) ((deg) * (float)M_PI / 180.0f)
#define RAD2DEG(rad) ((rad) * 180.0f / (float)M_PI)

#define LANCZOS_TAB_SIZE        3
#define LANCZOS_FAST_SCALE      100
#define LANCZOS_FAST_MAX_SIZE   (LANCZOS_TAB_SIZE << 1)
#define lanczos_coef(x)         m_lanczosCoef[(int)(fabs(x) * LANCZOS_FAST_SCALE + 0.5)]

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
    : m_ifilter(IF_NEAREST)
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
        double srcVal = CalcCartVal(cartPoint, src);
        double dstVal = CalcCartVal(cartPoint, dst);

        double diff = pow(dstVal - srcVal, 2);

        diffVals.push_back(diff);
    }

    GenerateR2SMap(src.Width(), src.Height());  // supposed that src is erp format!

    SPSNRDistortionMap* distortionMap = new SPSNRDistortionMap;
    cv::Mat& diffMap = distortionMap->m_diffMap;
    diffMap.create(src.Height(), src.Width(), CV_64FC1);

    for (int i = 0; i< diffMap.rows; i++) {
        for (int j = 0; j< diffMap.cols; j++) {
            int index = m_r2sMap.at<int>(i, j);
            assert(index >= 0 && index < diffVals.size());
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

void SPSNRDistortion::GenerateR2SMap(int width, int height, String mapFile)
{
    if (!m_r2sMap.empty())
        return;

    if (!mapFile.empty()) {
        cv::FileStorage fs(mapFile, cv::FileStorage::READ);
        if (fs.isOpened()) {
            fs["r2sMap"] >> m_r2sMap;
            return;
        }
    }
    
    m_r2sMap.create(height, width, CV_32SC1);

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
            m_r2sMap.at<int>(i, j) = index[0];
        }
    }
}

void SPSNRDistortion::SetIFilter(int ifilter)
{
    m_ifilter = ifilter;

    if (m_ifilter == IF_LANCZOS)
        InitLanczosCoef();
}

void SPSNRDistortion::SphToCart(const cv::Point2f& in, cv::Point3f& out) const
{
    float lat = DEG2RAD(in.x);
    float lon = DEG2RAD(in.y);

    out.x = cosf(lat) * cosf(lon);
    out.y = sinf(lat);
    out.z = -cosf(lat) * sinf(lon);
}

void SPSNRDistortion::CartToRect(const cv::Point3f& in, int w, int h, cv::Point2f& out) const
{
    float lambda = atan2f(-in.z, in.x);
    float phi = asinf(in.y);

    float u = lambda / (float)M_2PI + 0.5f;
    float v = 0.5f - phi / (float)M_PI;

    out.x = w * u - 0.5f;
    out.y = h * v - 0.5f;
}

void SPSNRDistortion::RectToCart(const cv::Point2f& in, int w, int h, cv::Point3f& out) const
{
    float lat = (float)(M_PI_2 - M_PI * in.y / h);
    float lon = (float)(M_2PI * in.x / w - M_PI);

    out.x = cosf(lat) * cosf(lon);
    out.y = sinf(lat);
    out.z = -cosf(lat) * sinf(lon);
}

void SPSNRDistortion::CartToCube(const cv::Point3f& in, const cv::Mat cmp, cv::Mat& face, cv::Point2f& out) const
{
    int w = cmp.cols;
    int h = cmp.rows;

    int A = w / 4;

    // ref JVET-D0021 
    std::vector<cv::Rect> faceArea({
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
    int faceIdx = -1;

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
    else if (aZ >= aX && aZ >= aY) {
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

    face = cmp(faceArea[faceIdx]);
    out.x = m;
    out.y = n;
}

double SPSNRDistortion::CalcCartVal(const cv::Point3f& cart, const Image& img)
{
    cv::Point2f planePt;
    cv::Mat face;

    switch (img.Format())
    {
    case GT_EQUIRECT:
        CartToRect(cart, img.Width(), img.Height(), planePt);
        face = img.Data();
        break;
    case GT_CUBEMAP:
        CartToCube(cart, img.Data(), face, planePt);
        break;
    default:
        assert(false);
        break;
    }

    if (false) {
        cv::namedWindow("face", cv::WINDOW_NORMAL);
        cv::imshow("face", face);
        cv::waitKey();
    }

    double val = 0.0;
    switch (m_ifilter)
    {
    case IF_NEAREST:
        val = IFilterNearest(face, planePt);
        break;
    case IF_LANCZOS:
        val = IFilterLanczos(face, planePt);
        break;
    default:
        assert(false);
        break;
    }

    return val;
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

void SPSNRDistortion::InitLanczosCoef()
{
    if (!m_lanczosCoef.empty())
        return;

    m_lanczosCoef.resize(LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE);

    for (int i = 0; i < LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE; i++)
    {
        float x = (float)i / LANCZOS_FAST_SCALE;
        m_lanczosCoef[i] = sinc(x) * sinc(x / LANCZOS_TAB_SIZE);
    }
}

double SPSNRDistortion::IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const
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
    double val = res / sum + 0.5;
    return val;
}
