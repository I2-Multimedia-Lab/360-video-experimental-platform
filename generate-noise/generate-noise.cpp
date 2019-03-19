// generate-noise.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define DEFAULT_SALT_PEPPER_NOISE_RATIO 0.001
#define DEFAULT_GAUSSIAN_NOISE_STDDEV 5

enum NoiseType
{
    NoiseType_None = 0,
    NoiseType_SaltPepper,
    NoiseType_Gaussian,
};

struct Option
{
    std::string _inputFile;
    std::string _outputFile;
    int _width;
    int _height;
    NoiseType _type;

    Option() 
    {
        Init();
    }

    void Init()
    {
        _width = _height = 0;
        _type = NoiseType_None;
    }

    bool IsValid()
    {
        return (!_inputFile.empty()) && (_width != 0) && (_height != 0) && (_type != NoiseType_None);
    }
};

bool ParseCmdLineArgs(int argc, char* argv[], Option& opt)
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
                opt._width = atoi(q);
            }
            else if (p[1] == 'h' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt._height = atoi(q);
            }
            else if (p[1] == 't' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt._type = (NoiseType)atoi(q);
            }
            else if (p[1] == 'i' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt._inputFile = q;
            }
            else if (p[1] == 'o' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt._outputFile = q;
            }
        }

    } while (false);

    if (opt._outputFile.empty() || opt._outputFile.compare(opt._inputFile) == 0) {
        opt._outputFile = opt._inputFile;
        opt._outputFile += ".out";
    }

    return opt.IsValid();
}

void Usage()
{
    char* help_str =
        "Usage: generate-noise.exe [options...] -i input_file -o output_file\n"
        "Option:\n"
        "   -w, width of video\n"
        "   -h, height of video\n"
        "   -t, type of noise, 1: salt noise, 2: gaussian noise\n"
        "   -i, input file name\n"
        "   -o, output file name\n"
        "\n"
        "Example: generate-noise.exe -w 720 -h 480 -t 2 -i input.yuv -o output.yuv\n";

    printf(help_str);
}


void AddSaltPepperNoise(cv::Mat& img, double ratio)
{
    cv::Mat saltPepperNoise = cv::Mat(img.size(), CV_32F);
    randu(saltPepperNoise, 0.0, 255.0);  // uniformly distributed

    img.setTo(255, saltPepperNoise > (255.0 * (1 - ratio)));  // white
    img.setTo(0, saltPepperNoise < (255.0 * ratio));  // black
}

void AddGuassianNoise(cv::Mat& img, int stddev)
{
    img.convertTo(img, CV_16S);

	cv::Mat gaussianNoise = cv::Mat(img.size(), img.type());
    cv::randn(gaussianNoise, 0, stddev);  // normally distributed 

    img = img + gaussianNoise;
    cv::normalize(img, img, 0, 255, CV_MINMAX, CV_8U);
}

int main(int argc, char* argv[])
{
    Option opt;
    if (!ParseCmdLineArgs(argc, argv, opt)) {
        Usage();
        return -1;
    }

    FILE* fpi = fopen(opt._inputFile.c_str(), "rb");
    if (fpi == NULL) {
        printf("Could not open input file %s.\n", opt._inputFile.c_str());
        return -1;
    }
    FILE* fpo = fopen(opt._outputFile.c_str(), "wb");
    if (fpo == NULL) {
        printf("Could not open output file %s.\n", opt._outputFile.c_str());
        fclose(fpi);
        return -1;
    }

    _fseeki64(fpi, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(fpi);
    int64_t frameSize = opt._width * opt._height * 3 / 2;
    int numFrames = (int)(fileSize / frameSize);

    _fseeki64(fpi, 0L, SEEK_SET);

    uint8_t* buffer = new uint8_t[frameSize];
    bool error = false;
    for (int i = 0; i < numFrames; i++) {
        fread(buffer, frameSize, 1, fpi);

        cv::Mat yuvImg;
        yuvImg.create(opt._height * 3 / 2, opt._width, CV_8UC1);
        memcpy(yuvImg.data, buffer, frameSize);
        if (yuvImg.empty()) {
            error = true;
            break;
        }
            
        cv::Mat rgbImg;
        cvtColor(yuvImg, rgbImg, CV_YUV2BGR_I420);

        if (opt._type == NoiseType_SaltPepper)
            AddSaltPepperNoise(rgbImg, DEFAULT_SALT_PEPPER_NOISE_RATIO);
        else if (opt._type == NoiseType_Gaussian)
            AddGuassianNoise(rgbImg, DEFAULT_GAUSSIAN_NOISE_STDDEV);

        cvtColor(rgbImg, yuvImg, CV_BGR2YUV_I420);
        memcpy(buffer, yuvImg.data, frameSize);
        fwrite(buffer, frameSize, 1, fpo);
        fflush(fpo);

        printf("\rProcessing... %.1lf%%",(i / (double)numFrames) * 100);
    }
    if (error) {
        printf("Oops, something wrong.\n");
    }
    else {
        printf("\rProcessing... 100%%\n");
        printf("Completed.\n");
    }
    
    delete[] buffer;

    fclose(fpi);
    fclose(fpo);

    return 0;
}

