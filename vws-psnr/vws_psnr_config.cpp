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
        "Usage: vws-psnr.exe [options...] reference_file distorted_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -f, frames per second, default 30 if not set\n"
        "\n"
        "Example: vws-psnr -w 4096 -h 2048 origin.yuv impaired.yuv\n";

    printf(help_str);
}

bool VWSPSNRConfig::ParseFromCmdLineArgs(int argc, char* argv[])
{
    if (argc <= 1 && argv == NULL)
        return false;

    bool haveSrc = false;
    bool haveDst = false;
    for (int i = 1; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] == '-') {
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
            else if (p[1] == 'f' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                m_fps = atoi(q);
            }
            continue;
        }
        
        if (!haveSrc) {  // reference file
            const char* q = argv[i];
            m_srcFilename = q;
            haveSrc = true;
            continue;
        }
        if (!haveDst) {  // processed file
            const char* q = argv[i];
            m_dstFilename = q;
            haveDst = true;
            continue;
        }

        //return false;
    }

    return IsValid();
}

bool VWSPSNRConfig::IsValid() const
{
    return !m_srcFilename.empty()
        && !m_dstFilename.empty()
        && m_width != 0 
        && m_height != 0;
}
