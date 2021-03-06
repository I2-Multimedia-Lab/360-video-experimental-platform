#pragma once

class Config
{
public:
    String m_srcFile;
    String m_dstFile;
    int m_srcWidth;
    int m_srcHeight;
    int m_dstWidth;
    int m_dstHeight;
    int m_srcFormat;
    int m_dstFormat;
    int m_ifilter;

    Config();
    ~Config();

    void Reset();
    bool IsValid();
    bool ParseCmdLineArgs(int argc, char* argv[]);

    static void Usage();
};
