// ws-psnr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct Config
{
    std::string m_srcFile;
    std::string m_dstFile;
    int m_width;
    int m_height;

    Config()
    {
        Init();
    }

    void Init()
    {
        m_width = m_height = 0;
    }

    bool IsValid()
    {
        return (!m_srcFile.empty()) && (!m_dstFile.empty()) && (m_width != 0) && (m_height != 0);
    }
};

bool ParseCmdLineArgs(int argc, char* argv[], Config& opt)
{
    opt.Init();

    do {
        for (int i = 0; i < argc; i++) {
            const char* p = argv[i];
            if (p[0] != '-')
                continue;

            if (p[1] == 'w' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_width = atoi(q);
            }
            else if (p[1] == 'h' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_height = atoi(q);
            }
            else if (p[1] == 'o' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_srcFile = q;
            }
            else if (p[1] == 'r' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_dstFile = q;
            }
        }

    } while (false);

    return opt.IsValid();
}

void Usage()
{
    char* help_str =
        "Usage: ws-psnr.exe [options...] -o reference_file -r processed_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -o, reference file name\n"
        "   -r, processed file name\n"
        "\n"
        "Example: ws-psnr.exe -w 720 -h 480 -t 2 -o origin.yuv -r impaired.yuv\n";

    printf(help_str);
}

class VideoSource
{
public:
    VideoSource(const char* filename, int width, int height)
        : m_filename(filename)
        , m_width(width)
        , m_height(height)
        , m_notYUV(false)
        , m_fp(NULL)
        , m_buffer(NULL)
    {
    }

    ~VideoSource()
    {
        Cleanup();
    }

    bool Init()
    {
        if (m_filename.substr(m_filename.length() - 4) != ".yuv")
            m_notYUV = true;
        
        if (!m_notYUV) {
            m_fp = fopen(m_filename.c_str(), "rb");
            if (m_fp == NULL)
                return false;

            int64_t yuvSize = m_width * m_height * 3 / 2;
            m_buffer = new uint8_t[yuvSize];
        }
        else {
            if (!m_vc.open(m_filename))
                return false;
        }

        return true;
    }

    void Cleanup()
    {
        if (m_fp != NULL) {
            fclose(m_fp);
            m_fp = NULL;
        }

        m_vc.release();

        delete[] m_buffer;
        m_buffer = NULL;
    }

    bool Read(cv::Mat& img)
    {
        int64_t yuvSize = m_width * m_height * 3 / 2;
        int64_t ySize = m_width * m_height;

        if (!m_notYUV) {
            if (fread(m_buffer, yuvSize, 1, m_fp) != 1)
                return false;
            img.create(m_height, m_width, CV_8UC1);
            memcpy(img.data, m_buffer, ySize);
        }
        else {
            if (!m_vc.read(img))
                return false;

            cv::cvtColor(img, img, cv::COLOR_BGRA2YUV_I420); // i cant figure out from ? to yuv420, just guess
            img = img(cv::Rect(0, 0, m_width, m_height));
        }

        return true;
    }

private:
    std::string m_filename;
    int m_width;
    int m_height;
    bool m_notYUV;
    FILE* m_fp;
    cv::VideoCapture m_vc;
    uint8_t* m_buffer;
};

int main(int argc, char* argv[])
{
    Config cfg;
    if (!ParseCmdLineArgs(argc, argv, cfg)) {
        Usage();
        return -1;
    }

    FILE* fp = fopen(cfg.m_srcFile.c_str(), "rb");
    if (fp == NULL) {
        printf("Could not open reference file %s.\n", cfg.m_srcFile.c_str());
        return -1;
    }
    _fseeki64(fp, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(fp);
    int64_t yuvSize = cfg.m_width * cfg.m_height * 3 / 2;
    int64_t ySize = cfg.m_width * cfg.m_height;
    int numFrames = (int)(fileSize / yuvSize);
    fclose(fp);
    fp = NULL;

    VideoSource vsSrc(cfg.m_srcFile.c_str(), cfg.m_width, cfg.m_height);
    if (!vsSrc.Init()) {
        printf("Reference file init failed.\n");
        return -1;
    }
    
    VideoSource vsDst(cfg.m_dstFile.c_str(), cfg.m_width, cfg.m_height);
    if (!vsDst.Init()) {
        printf("Reference file init failed.\n");
        return -1;
    }

    cv::Mat weightMap = cv::Mat::ones(cfg.m_height, cfg.m_width, CV_64FC1);
    for (int j = 0; j < weightMap.rows; j++) {
        double weight = cos((j - (cfg.m_height / 2 - 0.5)) * M_PI / cfg.m_height);
        weightMap.row(j) = weightMap.row(j) * weight;
    }

    uint8_t* buffer = new uint8_t[yuvSize];
    bool error = false;
    double total = 0.0;
    for (int i = 0; i < numFrames; i++) {
        cv::Mat srcImg;
        vsSrc.Read(srcImg);

        cv::Mat dstImg;
        vsDst.Read(dstImg);

        cv::Mat diffMap;
        cv::absdiff(srcImg, dstImg, diffMap);
        diffMap.convertTo(diffMap, CV_64FC1);
        cv::pow(diffMap, 2, diffMap);

        cv::Mat weightedDiff = diffMap.mul(weightMap, 100000);

        double WMSE = cv::sum(weightedDiff)[0] / cv::sum(weightMap)[0] / 100000;
        double WSPSNR = 10 * log10(255 * 255 / (WMSE + DBL_EPSILON));
        printf("Frame %d: %.4lf\n", i, WSPSNR);

        total += WSPSNR;
    }

    printf("Average: %.2lf\n", total / numFrames);

    return 0;
}

