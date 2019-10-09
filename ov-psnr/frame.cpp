#include "stdafx.h"
#include "frame.h"

Frame::Frame()
    : m_tubeRows(0)
    , m_tubeCols(0)
    , m_distortion(0.0)
    , m_psnr(0.0)
{

}

Frame::~Frame()
{

}

void Frame::ConstructTubes(const Segment& segment, int fps)
{
    const Deque<std::pair<Image, Image>>& frameQueue = segment.m_frameQueue;
    assert(frameQueue.size() != 0);

    int frameHeight = frameQueue[0].first.Height();
    int frameWidth = frameQueue[0].first.Width();

    m_tubeRows = frameHeight / Block::BLOCK_SIZE; // vertical
    m_tubeCols = frameWidth / Block::BLOCK_SIZE; // horizontal

    int newSize = m_tubeRows * m_tubeCols;
    m_tubes.resize(newSize);

    int interval = 1000 / fps;

    for (int k = 0; k < m_tubeRows; k++) { // row
        for (int l = 0; l < m_tubeCols; l++) { // col
            int x = l * Block::BLOCK_SIZE;
            int y = k * Block::BLOCK_SIZE;

            int index = k * m_tubeCols + l;
            Tube& t = m_tubes.at(index);
            t.Create(frameQueue, Point(x, y), interval);
        }
    }
}

void Frame::Calculate(const Segment& segment)
{
    for (int k = 0; k < m_tubeRows; k++) {
          for (int l = 0; l < m_tubeCols; l++) {
            Tube& t = GetTube(k, l);
            t.Calculate(segment);
        }
    }

    m_distortion = 0.0;
    for (auto& t : m_tubes) {
        double d = t.GetDistortion();
        d = pow(d, 2.0);
        m_distortion += d;
    }
    m_distortion /= m_tubes.size();
    m_distortion = sqrt(m_distortion);

    m_psnr = 10 * log10(255 * 255 / (m_distortion + DBL_EPSILON));
}

double Frame::GetDistortion() const
{
    return m_distortion;
}

double Frame::GetPSNR() const
{
    return m_psnr;
}

Tube& Frame::GetTube(int k, int l)
{
    int index = k * m_tubeCols + l;
    assert(index < m_tubes.size());

    return m_tubes.at(index);
}
