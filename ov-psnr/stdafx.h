// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <string>
using String = std::string;

#include <vector>
template <typename T>
using Vector = std::vector<T>;

#include <deque>
template <typename T>
using Deque = std::deque<T>;

#define _USE_MATH_DEFINES
#include <cmath>

#include <opencv2/opencv.hpp>
using Rect = cv::Rect;
using Point = cv::Point;

#include <opencv2/flann.hpp>
