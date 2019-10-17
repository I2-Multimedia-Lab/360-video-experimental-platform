#include "stdafx.h"
#include "video_source.h"
#include "cpppsnr_metric.h"

#define LANCZOS_TAB_SIZE        3
#define LANCZOS_FAST_SCALE      100
#define LANCZOS_FAST_MAX_SIZE   (LANCZOS_TAB_SIZE << 1)
#define lanczos_coef(x)         m_lanczosCoef[(int)(fabs(x) * LANCZOS_FAST_SCALE + 0.5)]

CPPPSNRMetric::CPPPSNRMetric()
    : m_globalPSNR(0.0)
{

}

CPPPSNRMetric::~CPPPSNRMetric()
{

}

bool CPPPSNRMetric::Init(int w, int h)
{
    printf("Generating CPP map...\n");
    GenerateCPPMap(w, h);

    printf("Initing Lanczos coef...\n");
    InitLanczosCoef();

    return true;
}

bool CPPPSNRMetric::Calc(VideoSource& src, VideoSource& dst)
{
    double globalMSE = 0.0;

    int numFrames = src.FrameCount();
    for (int i = 0; i < numFrames; i++) {
        Image srcImg;
        if (!src.ReadNextFrame(srcImg))
            break;

        Image dstImg;
        if (!dst.ReadNextFrame(dstImg))
            break;

        Image srcCPP;
        ERPToCPP(srcImg, srcCPP);

        Image dstCPP;
        ERPToCPP(dstImg, dstCPP);

        double mse = MSE(srcCPP, dstCPP);
        globalMSE += mse;

        printf("\r(%d)", i + 1);
    }
    printf("\r\n");

    globalMSE /= numFrames;
    m_globalPSNR = PSNR(globalMSE);

    return true;
}

void CPPPSNRMetric::Output()
{
    printf("Global PSNR: %lf\n", m_globalPSNR);
}

void CPPPSNRMetric::GenerateCPPMap(int w, int h)
{
    if (!m_cppMap.empty())
        return;

    m_cppMap = cv::Mat::zeros(h, w, CV_8UC1);

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            double phi = 3 * asin((double)j / h - 0.5);
            double lambda = (2 * M_PI * (double)i / w - M_PI) / (2 * cos(2 * phi / 3) - 1);

            double x = w * (lambda + M_PI) / (2 * M_PI);
            double y = h * (phi + M_PI / 2) / M_PI;

            int idx_x = (int)((x < 0) ? x - 0.5 : x + 0.5);
            int idx_y = (int)((y < 0) ? y - 0.5 : y + 0.5);

            if (idx_y >= 0 && idx_x >= 0 && idx_x < w && idx_y < h)
                m_cppMap.at<unsigned char>(j, i) = 1;
        }
    }
}

void CPPPSNRMetric::ERPToCPP(const Image& erp, Image& cpp)
{
    assert(!m_cppMap.empty());

    cpp = cv::Mat::zeros(m_cppMap.rows, m_cppMap.cols, CV_64FC1);

    for (int j = 0; j < cpp.rows; j++) {
        for (int i = 0; i < cpp.cols; i++) {
            double phi = 3 * asin((double)j / erp.rows - 0.5);
            double lambda = (2 * M_PI * (double)i / erp.cols - M_PI) / (2 * cos(2 * phi / 3) - 1);

            double x = erp.cols * (lambda + M_PI) / (2 * M_PI);
            double y = erp.rows * (phi + M_PI / 2) / M_PI;

            if (m_cppMap.at<uchar>(j, i) != 0)
                cpp.at<double>(j, i) = IFilterLanczos(erp, cv::Point2d(x, y));
        }
    }
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
