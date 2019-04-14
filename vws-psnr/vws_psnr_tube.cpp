#include "stdafx.h"
#include "vws_psnr_tube.h"

#define SEARCH_PARAM 7
#define DISTORTION_GRADIENT_THRESHOLD 10

VWSPSNRTube::VWSPSNRTube()
    : m_distortion(0.0)
{

}

VWSPSNRTube::~VWSPSNRTube()
{

}

void VWSPSNRTube::Cleanup()
{
    m_blocks.clear();
    m_distortion = 0.0;
}

bool VWSPSNRTube::Create(const std::deque<cv::Mat>& frameQueue, const cv::Rect firstBlockRect)
{
    cv::Mat preFrame, curFrame;
    cv::Rect blockRect;
    for (std::deque<cv::Mat>::const_reverse_iterator it = frameQueue.rbegin(); it != frameQueue.rend(); it++) {
        curFrame = *it;

        if (preFrame.empty())
            blockRect = firstBlockRect;
        else
            MatchBlock(curFrame, preFrame, blockRect);
        
        m_blocks.push_front(blockRect);

        preFrame = curFrame;
    }

    return true;
}

bool VWSPSNRTube::MatchBlock(const cv::Mat& c, const cv::Mat& r, cv::Rect& rc)
{
    // NTSS block matching
    int stepMax = (int)pow(2, (floor(log10(SEARCH_PARAM + 1) / log10(2)) - 1));

    int step = stepMax;

    int x = rc.x;
    int y = rc.y;

    // check 8 locations at distance = stepMax, generally 4
    cv::Mat1d diff1 = Match(c, r, cv::Point(x, y), cv::Point(x, y), step);

    double min1;
    cv::Point min1Loc;
    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);

    // check 8 locations at distance = 1
    cv::Mat1d diff2 = Match(c, r, cv::Point(x, y), cv::Point(x, y), 1);

    double min2;
    cv::Point min2Loc;
    cv::minMaxLoc(diff2, &min2, NULL, &min2Loc, NULL);

    // find the minimum amongst these 17 points (origin is repeated)
    cv::Point minLoc;
    double minDiff;
    bool ntssFlag = false;
    if (min1 < min2) {
        minLoc.x = x + (min1Loc.x - 1) * step;
        minLoc.y = y + (min1Loc.y - 1) * step;
        minDiff = min1;
        ntssFlag = false;
    }
    else {
        minLoc.x = x + (min2Loc.x - 1);
        minLoc.y = y + (min2Loc.y - 1);
        minDiff = min2;
        ntssFlag = true;
    }

    // first-step-stop
    if (minLoc.x == x && minLoc.y == y) {
        rc = cv::Rect(minLoc.x, minLoc.y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE); // stay origin point
        return true;
    }

    // second-step-stop
    if (ntssFlag) {
        diff2 = Match(c, r, cv::Point(x, y), minLoc, 1, true);

        cv::minMaxLoc(diff2, &min2, NULL, &min2Loc, NULL);

        if (min2 < minDiff) {
            minLoc.x += (min1Loc.x - 1);
            minLoc.y += (min1Loc.y - 1);
        }

        rc = cv::Rect(minLoc.x, minLoc.y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE);
        return true;
    }

    // do normal TSS 
    step /= 2;

    diff1 = Match(c, r, cv::Point(x, y), minLoc, step, true);

    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
    if (minDiff < min1) { // found and stop
        rc = cv::Rect(minLoc.x, minLoc.y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE);
        return true;
    }
    else {
        minLoc.x += (min1Loc.x - 1) * step;
        minLoc.y += (min1Loc.y - 1) * step;
        minDiff = min1;
    }

    // last step search
    step /= 2;
    diff1 = Match(c, r, cv::Point(x, y), minLoc, step, true);

    cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
    if (min1 < minDiff) {
        minLoc.x += (min1Loc.x - 1) * step;
        minLoc.y += (min1Loc.y - 1) * step;
        minDiff = min1;
    }
    rc = cv::Rect(minLoc.x, minLoc.y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE);

    return true;
}

cv::Mat1d VWSPSNRTube::Match(const cv::Mat& c, const cv::Mat& r, const cv::Point& center, const cv::Point& target, int step /*= 1*/, bool skipNOC /*= false*/)
{
    cv::Mat1d diff = cv::Mat1d::ones(3, 3);
    diff *= DBL_MAX;

    int x = center.x;
    int y = center.y;

    cv::Mat cb(c, cv::Rect(x, y, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE));

    int w = c.cols;
    int h = c.rows;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int rx = target.x - step + j * step;
            int ry = target.y - step + i * step;

            if (rx < 0 || ry < 0 || rx >= w - VWSPSNRBlock::BLOCK_SIZE || ry >= h - VWSPSNRBlock::BLOCK_SIZE)  // out of bounds
                continue;

            if (skipNOC && rx >= x - 1 && rx <= x + 1 && ry >= y - 1 && ry <= y + 1) // avoid recalculating neighbors of center
                continue;

            cv::Mat rb(r, cv::Rect(rx, ry, VWSPSNRBlock::BLOCK_SIZE, VWSPSNRBlock::BLOCK_SIZE));
            diff[i][j] = MAD(cb, rb);
        }
    }

    return diff;
}

double VWSPSNRTube::MAD(cv::InputArray s1, cv::InputArray s2)
{
    CV_Assert(s1.depth() == CV_8U && s2.depth() == CV_8U);

    double mad = cv::norm(s1, s2, cv::NORM_L1) / (s1.total() * s1.channels());
    return mad;
}

double VWSPSNRTube::Compute(const std::deque<cv::Mat>& diffMapQueue, const cv::Mat& weightMap, int interval)
{
    assert(diffMapQueue.size() == m_blocks.size());

    double s;
    double pd, cd;
    for (size_t i = 0; i < m_blocks.size(); i++) {
        VWSPSNRBlock& block = m_blocks[i];
        const cv::Mat& diffMap = diffMapQueue[i];

        cd = block.Compute(diffMap, weightMap);

        //  recursive filter
        if (i == 0) {
            s = cd;
        }
        else {
            double gradient = abs(cd - pd) * 1000 / interval;
            double alpha = (gradient > DISTORTION_GRADIENT_THRESHOLD) ? (interval / 200.0) : (interval / 400.0); // decided by distortion gradient 
            s = alpha * cd + (1 - alpha) * s;
        }

        pd = cd;
    }
    m_distortion = s;

    return m_distortion;
}

double VWSPSNRTube::GetDistortion() const
{
    return m_distortion;
}
