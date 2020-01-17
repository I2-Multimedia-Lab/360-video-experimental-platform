#pragma once

#include "common.h"

class SPSNRDistortionMap : public DistortionMap
{
public:
    friend class SPSNRDistortion;

    SPSNRDistortionMap();
    virtual ~SPSNRDistortionMap();

    virtual double GetBlcokDistortion(const Rect& rc) const;

private:
    cv::Mat m_diffMap;
};

class SPSNRDistortion
{
public:
    SPSNRDistortion();
    ~SPSNRDistortion();

    SPSNRDistortionMap* Calculate(const Image& src, const Image& dst);

    bool ReadSphPoint(const String& sphPointFile);
    void GenerateR2SMap(int width, int height);

private:
    void SphToCart(const cv::Point2f& in, cv::Point3f& out) const;
    void CartToSph(const cv::Point3f& in, cv::Point2f& out) const;
    void CartToRect(const cv::Point3f& in, int w, int h, cv::Point2f& out) const;
    void RectToCart(const cv::Point2f& in, int w, int h, cv::Point3f& out) const;
    double IFilterNearest(const cv::Mat& img, const cv::Point2f& in) const;
    float Clamp(float v, float low, float high) const;

private:
    Vector<cv::Point3f> m_cartPoints;
    cv::flann::Index m_kdtree;
    cv::Mat m_r2sMap;  // rect point to sphere point map 
};