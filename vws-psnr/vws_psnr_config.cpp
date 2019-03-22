#include "stdafx.h"
#include "vws_psnr_config.h"

#define DEFAULT_FPS 30

VWSPSNRConfig::VWSPSNRConfig()
    : m_width(0)
    , m_height(0)
    , m_fps(DEFAULT_FPS)
{
}


VWSPSNRConfig::~VWSPSNRConfig()
{
}

void VWSPSNRConfig::Usage()
{
    char* help_str =
        "Usage: vws-psnr.exe [options...] -t target_file -r ref_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -t, target file name\n"
        "   -r, reference file name\n"
        "   -f, frames per second, default 30 if not set\n"
        "\n"
        "Example: vws-psnr -w 720 -h 480 -t target.yuv -r origin.yuv\n";

    printf(help_str);
}

bool VWSPSNRConfig::ParseFromCmdLineArgs(int argc, char* argv[])
{
    if (argc <= 1 && argv == NULL)
        return false;

    for (int i = 0; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] != '-')
            continue;

        if (p[1] == 'w' && p[2] == '\0') {
            i++;
            const char* q = argv[i];
            m_width = atoi(q);
        }
        else if (p[1] == 'h' && p[2] == '\0') {
            i++;
            const char* q = argv[i];
            m_height = atoi(q);
        }
        else if (p[1] == 't' && p[2] == '\0') {
            i++;
            const char* q = argv[i];
            m_tFilename = q;
        }
        else if (p[1] == 'r' && p[2] == '\0') {
            i++;
            const char* q = argv[i];
            m_rFilename = q;
        }
        else if (p[1] == 'f' && p[2] == '\0') {
            i++;
            const char* q = argv[i];
            m_fps = atoi(q);
        }
    }

    return IsValid();
}

bool VWSPSNRConfig::IsValid() const
{
    return !m_tFilename.empty() 
        && !m_rFilename.empty() 
        && m_width != 0 
        && m_height != 0;
}
