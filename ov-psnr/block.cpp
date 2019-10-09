#include "stdafx.h"
#include "block.h"
#include "segment.h"

Block::Block(const Rect& rc)
    : m_rect(rc)
{

}

Block::~Block()
{

}

double Block::Calculate(const DistortionMap* dMap) const
{
    return dMap->GetBlcokDistortion(m_rect);
}
