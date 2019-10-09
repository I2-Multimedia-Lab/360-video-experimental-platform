#pragma once

#include "image.h"
#include "common.h"

class Block
{
public:
    Block(const Rect& rc);
    ~Block();

    static const int BLOCK_SIZE = 16;

    double Calculate(const DistortionMap* dMap) const;

private:
    Rect m_rect;
};
