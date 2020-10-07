#include "stdafx.h"
#include "video_source.h"
#include "cpppsnr_metric.h"
#include "common.h"

#define LANCZOS_TAB_SIZE        3
#define LANCZOS_FAST_SCALE      100
#define LANCZOS_FAST_MAX_SIZE   (LANCZOS_TAB_SIZE << 1)
#define lanczos_coef(x)         m_lanczosCoef[(int)(fabs(x) * LANCZOS_FAST_SCALE + 0.5)]

CPPPSNRMetric::CPPPSNRMetric()
    : m_globalPSNR(0.0)
    , m_ifilter(IF_NEAREST)
    , m_duration(0.0)
{

}

CPPPSNRMetric::~CPPPSNRMetric()
{

}

bool CPPPSNRMetric::Init(int w, int h, int ifilter)
{
    printf("Generating CPP map...\n");
    GenerateCPPMap(w, h);

    m_ifilter = ifilter;

    printf("Initing Lanczos coef...\n");
    InitLanczosCoef();

    return true;
}

bool CPPPSNRMetric::Calc(VideoSource& src, VideoSource& dst)
{
    double globalMSE = 0.0;
    m_duration = 0.0;

    int numFrames = src.FrameCount();
    for (int i = 0; i < numFrames; i++) {
        Image srcImg;
        if (!src.ReadNextFrame(srcImg))
            break;

        Image dstImg;
        if (!dst.ReadNextFrame(dstImg))
            break;

        clock_t start = clock();

        Image srcCPP;
        ConvertToCPP(src.Format(), srcImg, srcCPP);

        Image dstCPP;
        ConvertToCPP(dst.Format(), dstImg, dstCPP);

        double mse = MSE(srcCPP, dstCPP);
        double psnr = PSNR(mse);

        double t = (double)(clock() - start) / CLOCKS_PER_SEC;

        globalMSE += mse;
        m_duration += t;

        printf("Frame %d: %.4lf, %.4lf\n", i, psnr, t);
    }

    globalMSE /= numFrames;
    m_globalPSNR = PSNR(globalMSE);

    m_duration /= numFrames;

    return true;
}

void CPPPSNRMetric::Output()
{
    printf("Global PSNR: %lf\n", m_globalPSNR);
    printf("Average Time: %lf\n", m_duration);
}

void CPPPSNRMetric::GenerateCPPMap(int w, int h)
{
    if (!m_cppMap.empty())
        return;

    m_cppMap = cv::Mat::zeros(h, w, CV_8UC1);

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            double phi = 3 * asin(0.5 - (double)j / h);
            double lambda = (2 * M_PI * (double)i / w - M_PI) / (2 * cos(2 * phi / 3) - 1);

            double x = w * (lambda / (2 * M_PI) + 0.5);
            double y = h * (0.5 - phi / M_PI);

            int idx_x = (int)((x < 0) ? x - 0.5 : x + 0.5);
            int idx_y = (int)((y < 0) ? y - 0.5 : y + 0.5);

            if (idx_y >= 0 && idx_x >= 0 && idx_x < w && idx_y < h)
                m_cppMap.at<unsigned char>(j, i) = 1;
        }
    }
}

void CPPPSNRMetric::ConvertToCPP(int format, const Image& img, Image& cpp)
{
    switch (format)
    {
    case GT_EQUIRECT:
        ERPToCPP(img, cpp);
        break;
    case GT_CUBEMAP:
        CMPToCPP(img, cpp);
        break;
    default:
        break;
    }

}

void CPPPSNRMetric::ERPToCPP(const Image& erp, Image& cpp)
{
    assert(!m_cppMap.empty());

    cpp = cv::Mat::zeros(m_cppMap.rows, m_cppMap.cols, CV_64FC1);

    for (int j = 0; j < cpp.rows; j++) {
        for (int i = 0; i < cpp.cols; i++) {
            double phi = 3 * asin(0.5 - (double)j / cpp.rows);
            double lambda = (2 * M_PI * (double)i / cpp.cols - M_PI) / (2 * cos(2 * phi / 3) - 1);

            double x = erp.cols * (lambda / (2 * M_PI) + 0.5) - 0.5;
            double y = erp.rows * (0.5 - phi / M_PI) - 0.5;

            if (m_cppMap.at<uchar>(j, i) != 0) {
                switch (m_ifilter)
                {
                case IF_NEAREST:
                    cpp.at<double>(j, i) = IFilterNearest(erp, cv::Point2d(x, y));
                    break;
                case IF_LANCZOS:
                    cpp.at<double>(j, i) = IFilterLanczos(erp, cv::Point2d(x, y));
                    break;
                default:
                    assert(false);
                    break;
                }
            }
        }
    }
}

void CPPPSNRMetric::CMPToCPP(const Image& cmp, Image& cpp)
{
    std::vector<Image> faces = std::move(ExtractCubeFace(cmp));

    assert(!m_cppMap.empty());

    cpp = cv::Mat::zeros(m_cppMap.rows, m_cppMap.cols, CV_64FC1);

    for (int j = 0; j < cpp.rows; j++) {
        for (int i = 0; i < cpp.cols; i++) {
            double phi = 3 * asin(0.5 - (double)j / cpp.rows);
            double lambda = (2 * M_PI * (double)i / cpp.cols - M_PI) / (2 * cos(2 * phi / 3) - 1);

            // convert to cart
            cv::Point3d cart;
            cart.x = cos(phi) * cos(lambda);
            cart.y = sin(phi);
            cart.z = -cos(phi) * sin(lambda);

            cv::Point2d facePt;
            int faceIdx;
            CartToCube(cmp, cart, facePt, faceIdx);

            if (m_cppMap.at<uchar>(j, i) != 0) {
                switch (m_ifilter)
                {
                case IF_NEAREST:
                    cpp.at<double>(j, i) = IFilterNearest(faces[faceIdx], facePt);
                    break;
                case IF_LANCZOS:
                    cpp.at<double>(j, i) = IFilterLanczos(faces[faceIdx], facePt);
                    break;
                default:
                    assert(false);
                    break;
                }
            }
        }
    }
}

void CPPPSNRMetric::CartToCube(const Image& img, const cv::Point3d& in, cv::Point2d& out, int& faceIdx)
{
    int w = img.cols;
    int h = img.rows;

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

    double aX = fabs(in.x);
    double aY = fabs(in.y);
    double aZ = fabs(in.z);

    double u, v;

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

    double m = (u + 1.0) * (A / 2.0) - 0.5;
    double n = (v + 1.0) * (A / 2.0) - 0.5;
    assert(m >= -1 && m < A + 1);
    assert(n >= -1 && n < A + 1);

    out.x = m;
    out.y = n;
}

std::vector<Image> CPPPSNRMetric::ExtractCubeFace(const Image& cmp)
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

double CPPPSNRMetric::MSE(const Image& src, const Image& dst)
{
    cv::Mat cppDiff;
    cv::absdiff(src, dst, cppDiff);
    cv::pow(cppDiff, 2, cppDiff);

    double sum = 0.0;
    int n = 0;
    for (int j = 0; j < cppDiff.rows; j++) {
        for (int i = 0; i < cppDiff.cols; i++) {
            if (m_cppMap.at<uchar>(j, i) != 0) {
                sum += cppDiff.at<double>(j, i);
                n++;
            }
        }
    }
    
    double mse = sum / n;

    return mse;
}

double CPPPSNRMetric::PSNR(double mse)
{
    return 10 * log10(255 * 255 / (mse + DBL_EPSILON));
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

float CPPPSNRMetric::Clamp(float v, float low, float high) const
{
    if (v < low) return low;
    else if (v > high) return high;
    else return v;
}

double CPPPSNRMetric::IFilterNearest(const cv::Mat& img, const cv::Point2f& in) const
{
    int w = img.cols;
    int h = img.rows;

    float x = Clamp(in.x - 0.5f, 0.0f, w - 1.0f);
    float y = Clamp(in.y - 0.5f, 0.0f, h - 1.0f);

    int lx = lrintf(x);
    int ly = lrintf(y);

    return (double)img.at<uchar>(ly, lx);
}

void CPPPSNRMetric::InitLanczosCoef()
{
    if (!m_lanczosCoef.empty())
        return;

    m_lanczosCoef.resize(LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE);

    for (int i = 0; i < LANCZOS_FAST_MAX_SIZE * LANCZOS_FAST_SCALE; i++) {
        float x = (float)i / LANCZOS_FAST_SCALE;
        m_lanczosCoef[i] = sinc(x) * sinc(x / LANCZOS_TAB_SIZE);
    }
}

double CPPPSNRMetric::IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const
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
