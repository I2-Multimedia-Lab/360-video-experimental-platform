#pragma once

#include "image.h"

enum MetricType
{
    MT_UNKNOWN = 0,
    MT_PSNR,
    MT_SPSNR,
    MT_CPPPSNR,
    MT_WSPSNR,
};

enum GeometryType
{
    GT_UNKNOWN = 0,
    GT_EQUIRECT,
    GT_CUBEMAP,
};

enum IFilterType
{
    IF_UNKNOWN = 0,
    IF_NEAREST,
    IF_LANCZOS,
};

struct DistortionMap
{
    virtual ~DistortionMap() {}

    virtual double GetBlcokDistortion(const Rect& rc) const = 0;
};

struct Segment
{
    Deque<std::pair<Image, Image>> m_frameQueue;
    Deque<DistortionMap*> m_distortionMapQueue;
};
