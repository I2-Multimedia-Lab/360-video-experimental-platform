// ssim.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct Config
{
	std::string m_srcFile;
	std::string m_dstFile;
	int m_width;
	int m_height;

	Config()
	{
		Init();
	}

	void Init()
	{
		m_width = m_height = 0;
	}

	bool IsValid()
	{
		return (!m_srcFile.empty()) && (!m_dstFile.empty()) && (m_width != 0) && (m_height != 0);
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
				opt.m_srcFile = q;
			}
			else if (p[1] == 'o' && p[2] == '\0') {
				i++;
				const char* q = argv[i];
				opt.m_dstFile = q;
			}
		}

	} while (false);

	return opt.IsValid();
}

void Usage()
{
	char* help_str =
		"Usage: ssim.exe [options...]\n"
		"Option:\n"
		"   -w, width of video\n"
		"   -h, height of video\n"
		"   -i, reference file name\n"
		"   -o, processed file name\n"
		"\n"
		"Example: ssim.exe -w 720 -h 480 -i origin.yuv -o impaired.yuv\n";

	printf(help_str);
}

class VideoSource
{
public:
	VideoSource(const char* filename, int width, int height)
		: m_filename(filename)
		, m_width(width)
		, m_height(height)
		, m_notYUV(false)
		, m_fp(NULL)
		, m_buffer(NULL)
	{
	}

	~VideoSource()
	{
		Cleanup();
	}

	bool Init()
	{
		if (m_filename.substr(m_filename.length() - 4) != ".yuv")
			m_notYUV = true;

		if (!m_notYUV) {
			m_fp = fopen(m_filename.c_str(), "rb");
			if (m_fp == NULL)
				return false;

			int64_t yuvSize = m_width * m_height * 3 / 2;
			m_buffer = new uint8_t[yuvSize];
		}
		else {
			if (!m_vc.open(m_filename, cv::CAP_FFMPEG))
				return false;
		}

		return true;
	}

	void Cleanup()
	{
		if (m_fp != NULL) {
			fclose(m_fp);
			m_fp = NULL;
		}

		m_vc.release();

		delete[] m_buffer;
		m_buffer = NULL;
	}

	bool Read(cv::Mat& img)
	{
		int64_t yuvSize = m_width * m_height * 3 / 2;
		int64_t ySize = m_width * m_height;

		if (!m_notYUV) {
			if (fread(m_buffer, yuvSize, 1, m_fp) != 1)
				return false;
			img.create(m_height, m_width, CV_8UC1);
			memcpy(img.data, m_buffer, ySize);
		}
		else {
			if (!m_vc.read(img))
				return false;

			cv::cvtColor(img, img, cv::COLOR_BGRA2YUV_I420); // i cant figure out from ? to yuv420, just guess
			img = img(cv::Rect(0, 0, m_width, m_height));
		}

		return true;
	}

private:
	std::string m_filename;
	int m_width;
	int m_height;
	bool m_notYUV;
	FILE* m_fp;
	cv::VideoCapture m_vc;
	uint8_t* m_buffer;
};

cv::Scalar getMSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
	const double C1 = 6.5025, C2 = 58.5225;
	/***************************** INITS **********************************/
	int d = CV_32F;

	cv::Mat I1, I2;
	i1.convertTo(I1, d);            // cannot calculate on one byte large values
	i2.convertTo(I2, d);

	cv::Mat I2_2 = I2.mul(I2);        // I2^2
	cv::Mat I1_2 = I1.mul(I1);        // I1^2
	cv::Mat I1_I2 = I1.mul(I2);        // I1 * I2

	/*************************** END INITS **********************************/

	cv::Mat mu1, mu2;                   // PRELIMINARY COMPUTING
	GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
	GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

	cv::Mat mu1_2 = mu1.mul(mu1);
	cv::Mat mu2_2 = mu2.mul(mu2);
	cv::Mat mu1_mu2 = mu1.mul(mu2);

	cv::Mat sigma1_2, sigma2_2, sigma12;

	GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;

	GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
	sigma2_2 -= mu2_2;

	GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
	sigma12 -= mu1_mu2;

	///////////////////////////////// FORMULA ////////////////////////////////
	cv::Mat t1, t2, t3;

	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigma12 + C2;
	t3 = t1.mul(t2);                 // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigma2_2 + C2;
	t1 = t1.mul(t2);                 // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

	cv::Mat ssim_map;
	divide(t3, t1, ssim_map);        // ssim_map =  t3./t1;

	cv::Scalar mssim = mean(ssim_map);   // mssim = average of ssim map
	return mssim;
}

int main(int argc, char* argv[])
{
	Config cfg;
	if (!ParseCmdLineArgs(argc, argv, cfg)) {
		Usage();
		return -1;
	}

	FILE* fp = fopen(cfg.m_srcFile.c_str(), "rb");
	if (fp == NULL) {
		printf("Could not open reference file %s.\n", cfg.m_srcFile.c_str());
		return -1;
	}
	_fseeki64(fp, 0L, SEEK_END);
	int64_t fileSize = _ftelli64(fp);
	int64_t yuvSize = cfg.m_width * cfg.m_height * 3 / 2;
	int64_t ySize = cfg.m_width * cfg.m_height;
	int numFrames = (int)(fileSize / yuvSize);
	fclose(fp);
	fp = NULL;

	VideoSource vsSrc(cfg.m_srcFile.c_str(), cfg.m_width, cfg.m_height);
	if (!vsSrc.Init()) {
		printf("Reference file init failed.\n");
		return -1;
	}

	VideoSource vsDst(cfg.m_dstFile.c_str(), cfg.m_width, cfg.m_height);
	if (!vsDst.Init()) {
		printf("Reference file init failed.\n");
		return -1;
	}

	bool error = false;
	double total = 0.0;
	for (int i = 0; i < numFrames; i++) {
		cv::Mat srcImg;
		vsSrc.Read(srcImg);

		cv::Mat dstImg;
		vsDst.Read(dstImg);

		double SSIM = getMSSIM(srcImg, dstImg)[0];

		printf("Frame %d: %.4lf\n", i, SSIM);

		total += SSIM;
	}

	printf("Average: %.4lf\n", total / numFrames);

    return 0;
}

