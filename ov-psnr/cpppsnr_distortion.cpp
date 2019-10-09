#include "stdafx.h"
#include "cpppsnr_distortion.h"

#define LANCZOS_TAB_SIZE        3
#define LANCZOS_FAST_SCALE      100
#define LANCZOS_FAST_MAX_SIZE   (LANCZOS_TAB_SIZE << 1)
#define lanczos_coef(x)         m_lanczosCoef[(int)(fabs(x) * LANCZOS_FAST_SCALE + 0.5)]

#define PAD_SIZE                16
#define PAD_MIN(a, b)           ((a) < (b) ? (a) : (b))

CPPPSNRDistortionMap::CPPPSNRDistortionMap()
{

}

CPPPSNRDistortionMap::~CPPPSNRDistortionMap()
{

}

double CPPPSNRDistortionMap::GetBlcokDistortion(const Rect& rc) const
{
    cv::Mat blockDiffMap(m_diffMap, rc);

    double MSE = (double)cv::sum(blockDiffMap)[0] / blockDiffMap.total();

    return MSE;
}

CPPPSNRDistortion::CPPPSNRDistortion()
{

}

CPPPSNRDistortion::~CPPPSNRDistortion()
{

}

void CPPPSNRDistortion::Init(int cppWidth, int cppHeight)
{
    InitLanczosCoef();
    GenerateCPPMap(cppWidth, cppHeight);
}

CPPPSNRDistortionMap* CPPPSNRDistortion::Calculate(const Image& src, const Image& dst)
{
    cv::Mat cppSrc;
    ConvertToCPP(src, cppSrc);

    cv::Mat cppDst;
    ConvertToCPP(dst, cppDst);

    cv::Mat cppDiff;
    cv::absdiff(cppSrc, cppDst, cppDiff);
    cv::pow(cppDiff, 2, cppDiff);

    PadCPPDiff(cppDiff);

    GenerateR2CMap(src.Width(), src.Height());

    CPPPSNRDistortionMap* distortionMap = new CPPPSNRDistortionMap;
    cv::Mat& diffMap = distortionMap->m_diffMap;
    diffMap.create(src.Height(), src.Width(), CV_64FC1);

    for (int i = 0; i< diffMap.rows; i++) {
        for (int j = 0; j< diffMap.cols; j++) {
            int x = (m_r2cMap.at<cv::Vec2w>(i, j))[0];
            int y = (m_r2cMap.at<cv::Vec2w>(i, j))[1];

            diffMap.at<double>(i, j) = cppDiff.at<double>(y, x);
        }
    }

    return distortionMap;
}

void CPPPSNRDistortion::GenerateCPPMap(int w, int h)
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

void CPPPSNRDistortion::ConvertToCPP(const Image& in, cv::Mat& cpp)
{
    switch (in.Format())
    {
    case GT_EQUIRECT:
        ERPToCPP(in.Data(), cpp);
        break;
    default:
        assert(false);  // TODO
        break;
    }
}

void CPPPSNRDistortion::ERPToCPP(const cv::Mat& erp, cv::Mat& cpp)
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

void CPPPSNRDistortion::PadCPPDiff(cv::Mat& cppDiff)
{
    assert(!m_cppMap.empty());

    for (int j = 0; j < cppDiff.rows; j++) {
        for (int i = 0; i < cppDiff.cols; i++) {
            if (m_cppMap.at<uchar>(j, i) == 1) {
                int cnt = PAD_MIN(PAD_SIZE, i);
                double v = cppDiff.at<double>(j, i);
                for (int k = 1; k <= cnt; k++)
                    cppDiff.at<double>(j, i-k) = v;

                while (i < cppDiff.cols && m_cppMap.at<uchar>(j, i) == 1)
                    i++;

                cnt = PAD_MIN(PAD_SIZE, cppDiff.cols - i);
                v = cppDiff.at<double>(j, i - 1);
                for (int k = 0; k < cnt; k++)
                    cppDiff.at<double>(j, i + k) = v;

                break;
            }
        }
    }
}

void CPPPSNRDistortion::GenerateR2CMap(int width, int height)
{
    assert(!m_cppMap.empty());

    if (!m_r2cMap.empty())
        return;

    int wERP = width;
    int hERP = height;
    int wCPP = m_cppMap.cols;
    int hCPP = m_cppMap.rows;

    m_r2cMap.create(hERP, wERP, CV_16UC2);

    for (int i = 0; i < hERP; i++) {
        for (int j = 0; j < wERP; j++) {
            double phi = M_PI * ((double)i / hERP - 0.5);
            double y = hCPP * (0.5 + sin(phi / 3));
            double x = (wCPP / 2) * (1 + ((4 * cos(2 * phi / 3)) - 2) * ((double)j / wERP - 0.5));

            ushort idx_x = (ushort)(x + 0.5);
            ushort idx_y = (ushort)(y + 0.5);

            m_r2cMap.at<cv::Vec2w>(i, j) = { idx_x, idx_y };
        }
    }
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

void CPPPSNRDistortion::InitLanczosCoef()
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

double CPPPSNRDistortion::IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const
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
