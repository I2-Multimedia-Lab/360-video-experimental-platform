#pragma once

class CPPPSNRMetric
{
public:
    CPPPSNRMetric();
    ~CPPPSNRMetric();

    bool Init(int w, int h);
    bool Calc(VideoSource& src, VideoSource& dst);
    void Output();

private:
    void GenerateCPPMap(int w, int h);
    void ERPToCPP(const Image& erp, Image& cpp);
    double MSE(const Image& src, const Image& dst);
    double PSNR(double mse);

    void InitLanczosCoef();
    double IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const;

private:
    cv::Mat m_cppMap;
    double m_globalPSNR;
    std::vector<double> m_lanczosCoef;
};
