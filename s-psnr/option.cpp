#include "stdafx.h"
#include "option.h"
#include "mapper.h"

Option::Option()
{
    Reset();
}

Option::~Option()
{

}

void Option::Reset()
{
    m_srcFile.clear();
    m_dstFile.clear();
    m_sphFile.clear();
    m_srcWidth = m_srcHeight = 0;
    m_dstWidth = m_dstHeight = 0;
    m_srcFormat = MF_UNKNOWN;
    m_dstFormat = MF_UNKNOWN;
}

bool Option::IsValid()
{
    return (!m_srcFile.empty()) && (!m_dstFile.empty())
        && (!m_sphFile.empty())
        && (m_srcWidth != 0) && (m_srcHeight != 0)
        && (m_dstWidth != 0) && (m_dstHeight != 0)
        && (m_srcFormat != MF_UNKNOWN) && (m_dstFormat != MF_UNKNOWN);
}

bool Option::ParseCmdLineArgs(int argc, char* argv[])
{
    if (argc <= 1 && argv == NULL)
        return false;

    bool haveSrc = false;
    bool haveDst = false;
    for (int i = 1; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] == '-') {
            if (p[1] == 'b' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_srcWidth = atoi(q);
            }
            else if (p[1] == 'm' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_srcHeight = atoi(q);
            }
            else if (p[1] == 'v' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_dstWidth = atoi(q);
            }
            else if (p[1] == 'n' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_dstHeight = atoi(q);
            }
            else if (p[1] == 'i' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_srcFormat = Mapper::StringToFormat(q);
            }
            else if (p[1] == 'o' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_dstFormat = Mapper::StringToFormat(q);
            }
            continue;
        }

        if (!haveSrc) {
            const char* q = argv[i];
            m_srcFile = q;
            haveSrc = true;
            continue;
        }

        if (!haveDst) {
            const char* q = argv[i];
            m_dstFile = q;
            haveDst = true;
            continue;
        }

        const char* q = argv[i];
        m_sphFile = q;
    }

    return IsValid();
}

void Option::Usage()
{
    char* help_str =
        "Usage: s-psnr.exe [options...] src_file dst_file sph_file \n"
        "Option:\n"
        "   -b, width of src video\n"
        "   -m, height of src video\n"
        "   -v, width of dst video\n"
        "   -n, height of dst video\n"
        "   -i, src file format: rect, cube\n"
        "   -o, dest file format: rect, cube\n"
        "\n"
        "Example: s-psnr.exe -b 4096 -m 2048 -v 2048 -n 1024 -i rect -o cube origin.yuv impaired.yuv sphere_655362.txt\n";

    printf(help_str);
}
