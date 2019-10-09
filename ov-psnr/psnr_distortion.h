#pragma once

#include "common.h"

class PSNRDistortionMap : public DistortionMap
{
public:
    friend class PSNRDistortion;

    PSNRDistortionMap();
    virtual ~PSNRDistortionMap();

    virtual double GetBlcokDistortion(const Rect& rc) const;

private:
    cv::Mat m_diffMap;
};

class PSNRDistortion
{
public:
    PSNRDistortion();
    ~PSNRDistortion();

    PSNRDistortionMap* Calculate(const Image& src, const Image& dst);
};
