#pragma once


class VWSPSNRConfig
{
public:
    VWSPSNRConfig();
    ~VWSPSNRConfig();

public:
    static void Usage();
    bool ParseFromCmdLineArgs(int argc, char* argv[]);
    bool IsValid() const;

public:
    std::string m_srcFilename;
    std::string m_dstFilename;
    int m_width;
    int m_height;
    int m_fps;
};
