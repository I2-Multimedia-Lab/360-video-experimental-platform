#include "stdafx.h"
#include "config.h"
#include "common.h"

#define DEFAULT_FPS 25

Config::Config()
{
    Reset();
}

Config::~Config()
{

}

void Config::Reset()
{
    m_srcFilename.clear();
    m_dstFilename.clear();
    m_srcWidth = 0;
    m_srcHeight = 0;
    m_dstWidth = 0;
    m_dstHeight = 0;
    m_srcFormat = GT_UNKNOWN;
    m_dstFormat = GT_UNKNOWN;
    m_fps = DEFAULT_FPS;
    m_metric = MT_PSNR;
}

bool Config::IsValid() const
{
    if (m_srcFilename.empty() || m_dstFilename.empty())
        return false;
    if (m_srcFormat == GT_UNKNOWN || m_dstFormat == GT_UNKNOWN)
        return false;
    if (m_srcWidth == 0 || m_srcHeight == 0)
        return false;
    if (m_dstWidth == 0 || m_dstHeight == 0)
        return false;
    if (m_fps == 0)
        return false;
    if (m_metric == MT_UNKNOWN)
        return false;
    if ((m_metric == MT_PSNR || m_metric == MT_WSPSNR) && (m_srcFormat != m_dstFormat) && (m_srcWidth != m_dstWidth) && (m_srcHeight != m_dstHeight))
        return false;
    if (m_metric == MT_SPSNR_NN && m_sphPointFile.empty())
        return false;

    return true;
}

bool Config::ParseCmdLineArgs(int argc, char* argv[])
{
    if (argc <= 1 && argv == NULL)
        return false;

    for (int i = 0; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] != '-')
            continue;
        
        if (p[1] == 'i' && p[2] == '\0') {  // -i
            i++;
            const char* q = argv[i];
            m_srcFilename = q;
        }
        else if (p[1] == 'o' && p[2] == '\0') {  // -o
            i++;
            const char* q = argv[i];
            m_dstFilename = q;
        }
        else if (p[1] == 'w' && p[2] == '\0') {  // -w
            i++;
            const char* q = argv[i];
            m_srcWidth = atoi(q);
        }
        else if (p[1] == 'h' && p[2] == '\0') {  // -h
            i++;
            const char* q = argv[i];
            m_srcHeight = atoi(q);
        }
        else if (p[1] == 'e' && p[2] == '\0') {  // -e
            i++;
            const char* q = argv[i];
            m_dstWidth = atoi(q);
        }
        else if (p[1] == 'j' && p[2] == '\0') {  // -j
            i++;
            const char* q = argv[i];
            m_dstHeight = atoi(q);
        }
        else if (p[1] == 's' && p[2] == '\0') {  // -s
            i++;
            const char* q = argv[i];
            m_srcFormat = atoi(q);
        }
        else if (p[1] == 'd' && p[2] == '\0') {  // -d
            i++;
            const char* q = argv[i];
            m_dstFormat = atoi(q);
        }
        else if (p[1] == 'f'&& p[2] == '\0') {  // -f
            i++;
            const char* q = argv[i];
            m_fps = atoi(q);
        }
        else if (p[1] == 'm' && p[2] == '\0') {  // -m
            i++;
            const char* q = argv[i];
            m_metric = atoi(q);
        }
        else if (p[1] == 'p' && p[2] == '\0') {  // -p
            i++;
            const char* q = argv[i];
            m_sphPointFile = q;
        }
    }

    if (!IsValid()) {
        Usage();
        return false;
    }

    return true;
}

void Config::Usage()
{
    char* help_str =
        "Usage: ov-psnr.exe [options...]\n"
        "Option:\n"
        "   -i, source video file path\n"
        "   -o, destination video file path\n"
        "   -w, width of source video\n"
        "   -h, height of source video\n"
        "   -e, width of destination video\n"
        "   -j, height of destination video\n"
        "   -s, format of source video, 1: ERP, 2: CMP\n"
        "   -d, format of destination video, 1: ERP, 2: CMP\n"
        "   -f, frames per second, default 30 if not set\n"
        "   -m, metric type, 1:PSNR, 2:S-PSNR-NN, 3:CPP-PSNR, 4:WS-PSNR\n"
        "   -p, spheric coordinates file for S-PSNR\n"
        "\n"
        "Example: ov-psnr.exe -i origin.yuv -w 4096 -h 2048 -s 1 -o impaired.yuv -e 4096 -j 2048 -d 1 -f 25 -m 3 -p sphere_655362.txt\n";

    printf(help_str);
}
