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
    void ConvertToCPP(int format, const Image& img, Image& cpp);
    double MSE(const Image& src, const Image& dst);
    double PSNR(double mse);

    void ERPToCPP(const Image& erp, Image& cpp);
    void CMPToCPP(const Image& cmp, Image& cpp);
    void CartToCube(const Image& img, const cv::Point3d& in, cv::Point2d& out, int& faceIdx);
    std::vector<Image> ExtractCubeFace(const Image& cmp);

    void InitLanczosCoef();
    double IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const;

private:
    cv::Mat m_cppMap;
    double m_globalPSNR;
    std::vector<double> m_lanczosCoef;
};
