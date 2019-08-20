// generate-noise.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define DEFAULT_SALT_PEPPER_NOISE_RATIO 0.001
#define DEFAULT_GAUSSIAN_NOISE_STDDEV 5

enum class NoiseType
{
    None = 0,
    SaltPepper,
    Gaussian,
};

struct Config
{
    std::string m_inputFile;
    std::string m_outputFile;
    int m_width;
    int m_height;
    NoiseType m_type;

    Config() 
    {
        Init();
    }

    void Init()
    {
        m_width = m_height = 0;
        m_type = NoiseType::None;
    }

    bool IsValid()
    {
        return (!m_inputFile.empty()) && (m_width != 0) && (m_height != 0) && (m_type != NoiseType::None);
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
            else if (p[1] == 't' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_type = (NoiseType)atoi(q);
            }
            else if (p[1] == 'i' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_inputFile = q;
            }
            else if (p[1] == 'o' && p[2] == '\0') {
                i++;
                const char* q = argv[i];
                opt.m_outputFile = q;
            }
        }

    } while (false);

    if (opt.m_outputFile.empty() || opt.m_outputFile.compare(opt.m_inputFile) == 0) {
        opt.m_outputFile = opt.m_inputFile;
        opt.m_outputFile += ".out";
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
    Config cfg;
    if (!ParseCmdLineArgs(argc, argv, cfg)) {
        Usage();
        return -1;
    }

    FILE* fpi = fopen(cfg.m_inputFile.c_str(), "rb");
    if (fpi == NULL) {
        printf("Could not open input file %s.\n", cfg.m_inputFile.c_str());
        return -1;
    }
    FILE* fpo = fopen(cfg.m_outputFile.c_str(), "wb");
    if (fpo == NULL) {
        printf("Could not open output file %s.\n", cfg.m_outputFile.c_str());
        fclose(fpi);
        return -1;
    }

    _fseeki64(fpi, 0L, SEEK_END);
    int64_t fileSize = _ftelli64(fpi);
    int64_t frameSize = cfg.m_width * cfg.m_height * 3 / 2;
    int numFrames = (int)(fileSize / frameSize);

    _fseeki64(fpi, 0L, SEEK_SET);

    uint8_t* buffer = new uint8_t[frameSize];
    bool error = false;
    for (int i = 0; i < numFrames; i++) {
        fread(buffer, frameSize, 1, fpi);

        cv::Mat yuvImg;
        yuvImg.create(cfg.m_height * 3 / 2, cfg.m_width, CV_8UC1);
        memcpy(yuvImg.data, buffer, frameSize);
        if (yuvImg.empty()) {
            error = true;
            break;
        }
            
        cv::Mat rgbImg;
        cvtColor(yuvImg, rgbImg, CV_YUV2BGR_I420);

        if (cfg.m_type == NoiseType::SaltPepper)
            AddSaltPepperNoise(rgbImg, DEFAULT_SALT_PEPPER_NOISE_RATIO);
        else if (cfg.m_type == NoiseType::Gaussian)
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

