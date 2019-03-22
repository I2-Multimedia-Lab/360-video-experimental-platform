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
    std::string m_tFilename;
    std::string m_rFilename;
    int m_width;
    int m_height;
    int m_fps;
};
