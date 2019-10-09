#pragma once

#include "image.h"

struct DistortionMap
{
    virtual ~DistortionMap() {}

    virtual double GetBlcokDistortion(const Rect& rc) const = 0;
};

struct DistortionCalc
{
    virtual ~DistortionCalc() {}

    virtual bool Init() = 0;
    virtual DistortionMap* Calculate(const Image& src, const Image& dst) = 0;
};

