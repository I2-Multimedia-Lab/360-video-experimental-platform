// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <string>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

#include <opencv2\opencv.hpp>

using String = std::string;
using Image = cv::Mat;
using Vec2f = std::vector<cv::Point2f>;
using Vec3f = std::vector<cv::Point3f>;
using Vec1b = std::vector<unsigned char>;
using Vec1d = std::vector<double>;
