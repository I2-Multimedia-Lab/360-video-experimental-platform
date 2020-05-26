#pragma once

#include "common.h"

class CPPPSNRDistortionMap : public DistortionMap
{
public:
    friend class CPPPSNRDistortion;

    CPPPSNRDistortionMap();
    virtual ~CPPPSNRDistortionMap();

    virtual double GetBlcokDistortion(const Rect& rc) const;

private:
    cv::Mat m_diffMap;
};

class CPPPSNRDistortion
{
public:
    CPPPSNRDistortion();
    ~CPPPSNRDistortion();

    void Init(int cppWidth, int cppHeight, int ifilter);
    CPPPSNRDistortionMap* Calculate(const Image& src, const Image& dst);

private:
    void GenerateCPPMap(int w, int h);
    void ConvertToCPP(const Image& in, cv::Mat& cpp);
    void ERPToCPP(const cv::Mat& erp, cv::Mat& cpp);
    void CMPToCPP(const cv::Mat& cmp, cv::Mat& cpp);
    void CartToCube(const cv::Point3f& in, const cv::Mat cmp, cv::Mat& face, cv::Point2f& out) const;
    void PadCPPDiff(cv::Mat& cppDiff);
    void GenerateR2CMap(int width, int height);

    float Clamp(float v, float low, float high) const;
    double IFilterNearest(const cv::Mat& img, const cv::Point2f& in) const;

    void InitLanczosCoef();
    double IFilterLanczos(const cv::Mat& img, const cv::Point2d& in) const;

private:
    cv::Mat m_cppMap;
    cv::Mat m_r2cMap;
    int m_ifilter;
    Vector<double> m_lanczosCoef;
};


