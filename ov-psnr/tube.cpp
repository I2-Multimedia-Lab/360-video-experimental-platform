#include "stdafx.h"
#include "tube.h"

#define DEFAULT_INTERVAL 40
#define SEARCH_PARAM 7
#define DISTORTION_GRADIENT_THRESHOLD 2.5
#define ALPHA_1 0.8
#define ALPHA_2 0.5
#define BETA 1.0

Tube::Tube()
    : m_interval(DEFAULT_INTERVAL)
    , m_distortion(0.0)
{

}

Tube::~Tube()
{

}

void Tube::Create(const Deque<std::pair<Image, Image>>& frameQueue, Point startPos, int interval)
{
    m_blocks.clear();
    m_distortion = 0.0;

    Point cur, pre;
    for (int i = (int)frameQueue.size() - 1; i >= 0; i--) {
        if (i == frameQueue.size() - 1)
            cur = startPos;
        else 
            MatchBlock(frameQueue[i + 1].first, frameQueue[i].first, pre, cur);

        Rect rc(cur.x, cur.y, Block::BLOCK_SIZE, Block::BLOCK_SIZE);
        m_blocks.emplace_front(Block(rc));

        pre = cur;
    }

    m_interval = interval;
}

bool Tube::MatchBlock(const Image& src, const Image& dst, const Point& from, Point& to)
{
    // NTSS block matching
    int stepMax = (int)pow(2, (floor(log10(SEARCH_PARAM + 1) / log10(2)) - 1));

    int step = stepMax;

    // check 8 locations at distance = stepMax, generally 4
    cv::Mat1d diff1 = Match(src.Data(), dst.Data(), from, from, step);

    double min1;
    cv::Point min1Loc;
    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);

    // check 8 locations at distance = 1
    cv::Mat1d diff2 = Match(src.Data(), dst.Data(), from, from, 1);

    double min2;
    cv::Point min2Loc;
    cv::minMaxLoc(diff2, &min2, NULL, &min2Loc, NULL);

    // find the minimum amongst these 17 points (origin is repeated)
    cv::Point minLoc;
    double minDiff;
    bool ntssFlag = false;
    if (min1 < min2) {
        minLoc.x = from.x + (min1Loc.x - 1) * step;
        minLoc.y = from.y + (min1Loc.y - 1) * step;
        minDiff = min1;
        ntssFlag = false;
    }
    else {
        minLoc.x = from.x + (min2Loc.x - 1);
        minLoc.y = from.y + (min2Loc.y - 1);
        minDiff = min2;
        ntssFlag = true;
    }

    // first-step-stop
    if (minLoc.x == from.x && minLoc.y == from.y) {
        to = minLoc;  // stay origin point
        return true;
    }

    // second-step-stop
    if (ntssFlag) {
        diff2 = Match(src.Data(), dst.Data(), from, minLoc, 1, true);

        cv::minMaxLoc(diff2, &min2, NULL, &min2Loc, NULL);

        if (min2 < minDiff) {
            minLoc.x += (min1Loc.x - 1);
            minLoc.y += (min1Loc.y - 1);
        }

        to = minLoc;
        return true;
    }

    // do normal TSS 
    step /= 2;

    diff1 = Match(src.Data(), dst.Data(), from, minLoc, step, true);

    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
    if (minDiff < min1) { // found and stop
        to = minLoc;
        return true;
    }
    else {
        minLoc.x += (min1Loc.x - 1) * step;
        minLoc.y += (min1Loc.y - 1) * step;
        minDiff = min1;
    }

    // last step search
    step /= 2;
    diff1 = Match(src.Data(), dst.Data(), from, minLoc, step, true);

    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
    if (min1 < minDiff) {
        minLoc.x += (min1Loc.x - 1) * step;
        minLoc.y += (min1Loc.y - 1) * step;
        minDiff = min1;
    }
    to = minLoc;

    return true;
}

cv::Mat1d Tube::Match(const cv::Mat& src, const cv::Mat& dst, const cv::Point& origin, const cv::Point& search, int step /*= 1*/, bool skipNOC /*= false*/)
{
    cv::Mat1d diff = cv::Mat1d::ones(3, 3);
    diff *= DBL_MAX;

    int ox = origin.x;
    int oy = origin.y;

    cv::Mat templateBlock(src, cv::Rect(ox, oy, Block::BLOCK_SIZE, Block::BLOCK_SIZE));

    int w = dst.cols;
    int h = dst.rows;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int sx = search.x - step + j * step;
            int sy = search.y - step + i * step;

            if (sx < 0 || sy < 0 || sx >= w - Block::BLOCK_SIZE || sy >= h - Block::BLOCK_SIZE)  // out of bounds
                continue;

            if (skipNOC && sx >= ox - 1 && sx <= ox + 1 && sy >= oy - 1 && sy <= oy + 1) // avoid recalculating neighbors of center
                continue;

            cv::Mat searchBlock(dst, cv::Rect(sx, sy, Block::BLOCK_SIZE, Block::BLOCK_SIZE));
            diff[i][j] = MAD(searchBlock, templateBlock);
        }
    }

    return diff;
}

double Tube::MAD(cv::InputArray s1, cv::InputArray s2)
{
    CV_Assert(s1.depth() == CV_8U && s2.depth() == CV_8U);

    double mad = cv::norm(s1, s2, cv::NORM_L1) / (s1.total() * s1.channels());
    return mad;
}

void Tube::Calculate(const Segment& segment)
{
    Deque<double> bds;  // distortion of blocks in tube
    for (size_t i = 0; i < m_blocks.size(); i++) {
        double bd = m_blocks[i].Calculate(segment.m_distortionMapQueue[i]);
        bds.push_back(bd);
    }

    double D, d, pd; // distortion
    double pg, cg, mg; // distortion gradient
    int sc = 0; // sign changes of the distortion gradient

    for (int i = 0; i < bds.size(); i++) {
        d = bds[i];

        //  recursive filter
        if (i == 0) {
            D = d;
            pg = cg = mg = 0.0;
        }
        else {
            cg = (d - pd) / m_interval;
            double acg = abs(cg);

            double alpha = (acg > DISTORTION_GRADIENT_THRESHOLD) ? ALPHA_1 : ALPHA_2; // decided by distortion gradient 
            D = (1 - alpha) * d + alpha * D;

            if ((cg < 0.0) != (pg < 0.0))
                sc++;
            if (acg > DISTORTION_GRADIENT_THRESHOLD && acg > mg)
                mg = acg;
        }

        pd = d;
        pg = cg;
    }

    double tdc = TemporalDistortionCoefficient(mg, sc);
    m_distortion = D * (1 + BETA * tdc);
}

double Tube::TemporalDistortionCoefficient(double mg, int sc)
{
    double g = 16;
    double mean = 1;
    double sigma = 6.2;
    return mg * g * exp(-pow((sc - mean), 2) / (2 * pow(sigma, 2))) / (sigma * sqrt(2 * M_PI));
}

double Tube::GetDistortion() const
{
    return m_distortion;
}
