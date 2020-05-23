#pragma once

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
