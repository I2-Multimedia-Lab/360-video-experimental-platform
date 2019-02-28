// ntss-block-matching.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define BLOCK_SIZE 

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
    int64_t frameSize = param._width * param._height * 3 / 2;
    int numFrames = (int)(fileSize / frameSize);

    _fseeki64(fp, 0L, SEEK_SET);

    uint8_t* buffer = new uint8_t[frameSize];
    bool error = false;
    do 
    {
        fread(buffer, frameSize, 1, fp);
        cv::Mat curFrame;
        curFrame.create(param._height * 3 / 2, param._width, CV_8UC1);
        memcpy(curFrame.data, buffer, frameSize);
        if (curFrame.empty()) {
            error = true;
            break;
        }

        for (int i = 1; i < numFrames; i++) {
            cv::Mat preFrame = curFrame.clone();

            fread(buffer, frameSize, 1, fp);
            curFrame.create(param._height * 3 / 2, param._width, CV_8UC1);
            memcpy(curFrame.data, buffer, frameSize);
            if (curFrame.empty()) {
                error = true;
                break;
            }
            


        }
    } while (false);
    
    if (error) {
        printf("Oops, something wrong.\n");
    }
    else {
        printf("Completed.\n");
    }

    delete[] buffer;

    fclose(fp);

    return 0;
}

