// ntss-block-matching.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define BLOCK_SIZE 16
#define SEARCH_PARAM 7

struct Parameter
{
    std::string _filename;
    int _width;
    int _height;

    Parameter()
    {
        Init();
    }

    void Init()
    {
        _width = _height = 0;
    }

    bool IsValid()
    {
        return (!_filename.empty()) && (_width != 0) && (_height != 0);
    }
};

bool LoadCmdlineParam(int argc, char* argv[], Parameter& param)
{
    param.Init();

    do {
        for (int i = 0; i < argc; i++) {
            const char* p = argv[i];
            if (p[0] != '-')
                continue;

            if (p[1] == 'w' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                param._width = atoi(q);
            }
            else if (p[1] == 'h' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                param._height = atoi(q);
            }
            else if (p[1] == 'i' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                param._filename = q;
            }
        }

    } while (false);

    return param.IsValid();
}

void Usage()
{
    char* help_str =
        "Usage: ntss-block-matching.exe [options...] -i input_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -i, input file name\n"
        "\n"
        "Example: ntss-block-matching -w 720 -h 480 -i test.yuv\n";

    printf(help_str);
}

double MAD(cv::InputArray s1, cv::InputArray s2)
{
    CV_Assert(s1.depth() == CV_8U && s2.depth() == CV_8U);

    double mad = cv::norm(s1, s2, cv::NORM_L1) / (s1.total() * s1.channels());
    return mad;
}

double MSE(cv::InputArray s1, cv::InputArray s2)
{
    CV_Assert(s1.depth() == CV_8U && s2.depth() == CV_8U);
    
    double mse = cv::norm(s1, s2, cv::NORM_L2SQR) / (s1.total() * s1.channels());
    return mse;
}

cv::Mat1d Match(const cv::Mat& c, const cv::Mat& r, const cv::Point& center, const cv::Point& target, int step = 1, bool skipNOC = false)
{
    cv::Mat1d diff = cv::Mat1d::ones(3, 3);
    diff *= DBL_MAX;

    int x = center.x;
    int y = center.y;

    cv::Mat cb(c, cv::Rect(x, y, BLOCK_SIZE, BLOCK_SIZE));

    int w = c.cols;
    int h = c.rows;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int rx = target.x - step + j * step;
            int ry = target.y - step + i * step;

            if (rx < 0 || ry < 0 || rx >= w - BLOCK_SIZE || ry >= h - BLOCK_SIZE)  // out of bounds
                continue;

            if (skipNOC && rx >= x - 1 && rx <= x + 1 && ry >= y - 1 && ry <= y + 1) // avoid recalculating neighbors of center
                continue;

            cv::Mat rb(r, cv::Rect(rx, ry, BLOCK_SIZE, BLOCK_SIZE));
            diff[i][j] = MAD(cb, rb);
        }
    }

    return diff;
}

bool EstimateMotionVector(const cv::Mat& c, const cv::Mat& r, cv::Mat& mv)
{
    CV_Assert(c.size() == r.size());

    int w = c.size().width;
    int h = c.size().height;

    int m = h / BLOCK_SIZE; // vertical
    int n = w / BLOCK_SIZE; // horizontal

    mv.create(m, n, CV_16UC2);

    int stepMax = (int)pow(2, (floor(log10(SEARCH_PARAM + 1) / log10(2)) - 1));  // step is calculated by search parameter p

    for (int i = 0; i < m; i++) { // row
        for (int j = 0; j < n; j++) { // col
            int x = j * BLOCK_SIZE;
            int y = i * BLOCK_SIZE;

            int step = stepMax;

            // check 8 locations at distance = stepMax, generally 4
            cv::Mat1d diff1 = Match(c, r, cv::Point(x, y), cv::Point(x, y), step);
            //std::cout << diff1 << std::endl;

            double min1;
            cv::Point min1Loc;
            cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);

            // check 8 locations at distance = 1
            cv::Mat1d diff2 = Match(c, r, cv::Point(x, y), cv::Point(x, y), 1);
            //std::cout << diff2 << std::endl;

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
                mv.at<cv::Vec2w>(i, j) = cv::Vec2w(x, y);  // stay origin point
                continue;
            }

            // second-step-stop
            if (ntssFlag) { 
                diff2 = Match(c, r, cv::Point(x, y), minLoc, 1, true);
                //std::cout << diff2 << std::endl;

                cv::minMaxLoc(diff2, &min2, NULL, &min2Loc, NULL);

                if (min2 < minDiff) {
                    minLoc.x += (min1Loc.x - 1);
                    minLoc.y += (min1Loc.y - 1);
                }
                
                mv.at<cv::Vec2w>(i, j) = cv::Vec2w(minLoc.x, minLoc.y);
                continue; 
            }

            // do normal TSS 
            step /= 2;

            diff1 = Match(c, r, cv::Point(x, y), minLoc, step, true);
            //std::cout << diff1 << std::endl;

            cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
            if (minDiff < min1) { // found and stop
                mv.at<cv::Vec2w>(i, j) = cv::Vec2w(minLoc.x, minLoc.y);
                continue;
            }
            else {
                minLoc.x += (min1Loc.x - 1) * step;
                minLoc.y += (min1Loc.y - 1) * step;
                minDiff = min1;
            }

            // last step search
            step /= 2;
            diff1 = Match(c, r, cv::Point(x, y), minLoc, step, true);
            //std::cout << diff1 << std::endl;

            cv::minMaxLoc(diff1, &min1, NULL, &min1Loc, NULL);
            if (min1 < minDiff) {
                minLoc.x += (min1Loc.x - 1) * step;
                minLoc.y += (min1Loc.y - 1) * step;
                minDiff = min1;
            }
            mv.at<cv::Vec2w>(i, j) = cv::Vec2w(minLoc.x, minLoc.y);
        }
    }

    return true;
}


int main(int argc, char* argv[])
{
    Parameter param;
    if (!LoadCmdlineParam(argc, argv, param)) {
        Usage();
        return -1;
    }
    
    FILE* fp = fopen(param._filename.c_str(), "rb");
    if (fp == NULL) {
        printf("Could not open video file %s.\n", param._filename.c_str());
        return -1;
    }

    _fseeki64(fp, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(fp);
    int64_t yuvSize = param._width * param._height * 3 / 2;
    int64_t ySize = param._width * param._height;
    int numFrames = (int)(fileSize / yuvSize);

    std::vector<cv::Mat> motionVectors(numFrames);

    _fseeki64(fp, 0L, SEEK_SET);

    uint8_t* buffer = new uint8_t[yuvSize];
    bool error = false;

    do 
    {
        fread(buffer, yuvSize, 1, fp);
        cv::Mat curFrame;
        curFrame.create(param._height, param._width, CV_8UC1);
        memcpy(curFrame.data, buffer, ySize);
        if (curFrame.empty()) {
            error = true;
            break;
        }
        
        size_t t = curFrame.total();
        int c = curFrame.channels();

        for (int i = 1; i < numFrames; i++) {
            cv::Mat preFrame = curFrame.clone();

            fread(buffer, yuvSize, 1, fp);
            memcpy(curFrame.data, buffer, ySize);
            if (curFrame.empty()) {
                error = true;
                break;
            }

            cv::Mat& mv = motionVectors[i];
            bool r = EstimateMotionVector(curFrame, preFrame, mv);

            printf("Frame %d\n", i);
        }
    } while (false);
    
    if (error) {
        printf("Oops, something wrong.\n");
    }
    else {
        std::fstream of("./mv.output", std::fstream::out | std::fstream::trunc);
        for (std::vector<cv::Mat>::const_iterator it = motionVectors.begin(); it != motionVectors.end(); it++) {
            of << *it << std::endl;
        }
        of.close();
        printf("Completed.\n");
    }

    delete[] buffer;
    fclose(fp);

    return 0;
}

