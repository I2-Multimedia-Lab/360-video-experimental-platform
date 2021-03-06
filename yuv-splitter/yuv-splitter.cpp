// yuv-splitter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct Config
{
    std::string m_if;
    std::string m_of;
    int m_width;
    int m_height;
    int m_startFrame;
    int m_endFrame;

    Config()
    {
        Init();
    }

    void Init()
    {
        m_width = m_height = 0;
        m_startFrame = 0;
        m_endFrame = -1;
    }

    bool IsValid()
    {
        return (!m_if.empty()) && (!m_of.empty()) && (m_width != 0) && (m_height != 0);
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
            else if (p[1] == 'i' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_if = q;
            }
            else if (p[1] == 'o' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_of = q;
            }
            else if (p[1] == 'f' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                int s, e;
                if (sscanf(q, "%d:%d", &s, &e) == 2) {
                    opt.m_startFrame = s;
                    opt.m_endFrame = e;
                }
            }
        }

    } while (false);

    return opt.IsValid();
}

void Usage()
{
    char* help_str =
        "Usage: yuv-splitter.exe [options...] -i input_file -o output_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -i, input file name\n"
        "   -o, output file name\n"
        "   -f x:x, frame range to be splitted (1 is first frame, -1 is the last)\n"
        "\n"
        "Example: yuv-splitter.exe -w 720 -h 480 -f 1:30 -i input.yuv -o output.yuv \n";

    printf(help_str);
}

int main(int argc, char* argv[])
{
    Config cfg;
    if (!ParseCmdLineArgs(argc, argv, cfg)) {
        Usage();
        return -1;
    }

    FILE* fpi = fopen(cfg.m_if.c_str(), "rb");
    if (fpi == NULL) {
        printf("Could not open input file %s.\n", cfg.m_if.c_str());
        return -1;
    }
    FILE* fpo = fopen(cfg.m_of.c_str(), "wb");
    if (fpo == NULL) {
        printf("Could not open output file %s.\n", cfg.m_of.c_str());
        fclose(fpi);
        return -1;
    }

    _fseeki64(fpi, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(fpi);
    int64_t frameSize = cfg.m_width * cfg.m_height * 3 / 2;
    int numFrames = (int)(fileSize / frameSize);

    if (cfg.m_startFrame < 1 
        || cfg.m_startFrame > numFrames 
        || (cfg.m_endFrame < 1 && cfg.m_endFrame != -1) 
        || cfg.m_endFrame > numFrames
        || (cfg.m_endFrame < cfg.m_startFrame && cfg.m_endFrame != -1)) {
        printf("Frame range is invalid.\n");
        fclose(fpi);
        fclose(fpo);
        return -1;
    }
    else {
        cfg.m_startFrame = cfg.m_startFrame - 1; // we count frames from 0, other than 1
        cfg.m_endFrame = cfg.m_endFrame == -1 ? numFrames - 1 : cfg.m_endFrame - 1;
    }

    _fseeki64(fpi, cfg.m_startFrame * frameSize, SEEK_SET);

    uint8_t* buffer = new uint8_t[frameSize];
    for (int i = cfg.m_startFrame; i <= cfg.m_endFrame; i++) {
        fread(buffer, frameSize, 1, fpi);
        fwrite(buffer, frameSize, 1, fpo);
        fflush(fpo);
        printf("\rFrame %d", i);
    }
    printf("\rCompleted.\n");

    delete[] buffer;

    fclose(fpi);
    fclose(fpo);

    return 0;
}

