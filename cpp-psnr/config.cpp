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
    m_srcFile.clear();
    m_dstFile.clear();
    m_srcWidth = 0;
    m_srcHeight = 0;
    m_dstWidth = 0;
    m_dstHeight = 0;
    m_srcFormat = GT_UNKNOWN;
    m_dstFormat = GT_UNKNOWN;
    m_ifilter = IF_NEAREST;
}

bool Config::IsValid()
{
    if (m_srcFile.empty() || m_dstFile.empty())
        return false;
    if (m_srcFormat == GT_UNKNOWN || m_dstFormat == GT_UNKNOWN)
        return false;
    if (m_srcWidth == 0 || m_srcHeight == 0)
        return false;
    if (m_dstWidth == 0 || m_dstHeight == 0)
        return false;

    if ((m_srcFormat == GT_CUBEMAP && m_srcHeight / (double)m_srcWidth != 0.75) ||
        (m_dstFormat == GT_CUBEMAP && m_dstHeight / (double)m_dstWidth != 0.75))  // Only support cube map 4x3 format!
        return false;

    if (m_ifilter != IF_NEAREST && m_ifilter != IF_LANCZOS)
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
            m_srcFile = q;
        }
        else if (p[1] == 'o' && p[2] == '\0') {  // -o
            i++;
            const char* q = argv[i];
            m_dstFile = q;
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
        else if (p[1] == 'l' && p[2] == '\0') {  // -l
            i++;
            const char* q = argv[i];
            m_ifilter = atoi(q);
        }
    }

    return IsValid();
}

void Config::Usage()
{
    char* help_str =
        "Usage: cpp-psnr.exe [options...]\n"
        "Option:\n"
        "   -i, source video file path\n"
        "   -o, destination video file path\n"
        "   -w, width of source video\n"
        "   -h, height of source video\n"
        "   -e, width of destination video\n"
        "   -j, height of destination video\n"
        "   -s, format of source video, 1:ERP, 2:CMP(4x3)\n"
        "   -d, format of destination video, 1:ERP, 2:CMP(4x3)\n"
        "   -l, interpolation type, 1:Nearest(default), 2:Lanczos\n"
        "\n"
        "Example: cpp-psnr.exe -i origin_erp.yuv -w 4096 -h 2048 -s 1 -o impaired_cmp.yuv -e 4096 -j 3072 -d 2 -l 2\n";

    printf(help_str);
}
