#pragma once

class Config
{
public:
    String m_srcFilename;
    String m_dstFilename;
    int m_srcWidth;
    int m_srcHeight;
    int m_dstWidth;
    int m_dstHeight;
    int m_srcFormat;
    int m_dstFormat;
    int m_fps;
    int m_metric;
    String m_sphPointFile;

public:
    Config();
    ~Config();

    void Reset();
    bool IsValid() const;
    bool ParseCmdLineArgs(int argc, char* argv[]);

    void Usage();
};
