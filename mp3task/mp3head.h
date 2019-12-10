#pragma once
#include <assert.h>
#include <iosfwd>
#include <cstdint>
#include <vector>
#include <fstream>
#include <memory>

class mp3head
{
private:
	unsigned int bitrate[16] = { 0,32000,40000,48000,56000,64000,80000,96000,
		112000,128000,160000,192000,224000,256000,320000,0 };
	unsigned int freq[4] = { 44100,48000,32000,00000 };
	double fAveBitRate;
	unsigned int nFrames = 1;
	unsigned int nSampleRate;
	struct mp3Header
	{
		unsigned long nBitrate;
		unsigned int nSampleRate;
		std::streampos spNextFrame;
	};
	std::ifstream fsInputMp3;
	bool checkvalidheader(uint32_t headerBits);
	std::streampos findnextframe(std::streampos startSearch);
	std::shared_ptr<mp3Header> getheader(uint32_t headBits, std::streampos curPos);
	std::shared_ptr<mp3Header> getheaderat(std::streampos possibleHeader);
	std::shared_ptr<mp3Header> getfirstheader();
	void getallheaders();
public:
	mp3head(std::string sFilename);
	double getavgbitrate();
	unsigned int getframecount();
	unsigned int getsamplerate();


};

