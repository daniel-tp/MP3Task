#include "mp3head.h"
#include <iosfwd>
#include <string>
#include <fstream>
#include <memory>
#include <iostream>

mp3head::mp3head(std::string sFilename)
{
	//Load the given mp3 file and process it
	fsInputMp3 = std::ifstream(sFilename.c_str(), std::ios::binary);
	if (!fsInputMp3)
	{
		throw std::invalid_argument("Invalid File, Cannot Open");
	}
	//Retrieve all headers and calculate the wanted values
	getallheaders();
}

double mp3head::getavgbitrate()
{
	return fAveBitRate;
}

unsigned int mp3head::getframecount()
{
	return nFrames;
}

unsigned int mp3head::getsamplerate()
{
	return nSampleRate;
}

//Check the given header bits seem to be valid
bool mp3head::checkvalidheader(uint32_t nHeaderBits)
{
	//Make sure this is a mpeg layer 3 header
	unsigned int uiLayerValue = (nHeaderBits >> 17) & 0x3;
	if (uiLayerValue != 1)
	{
		return false;
	}
	//Make sure nBitrate index is not invalid
	unsigned int uiBitrateIndex = (nHeaderBits >> 12) & 0xF;
	if (uiBitrateIndex == 16)
	{
		return false;
	}
	//Make sure sample rate index is not invalid
	unsigned int uiSampleIndex = (nHeaderBits >> 10) & 0x3;
	if (uiSampleIndex == 3)
	{
		return false;
	}

	return true;
}
//Make a header out of given header bits
std::shared_ptr<mp3head::mp3Header> mp3head::getheader(uint32_t nHeaderBits, std::streampos spCurrentPosition)
{
	auto pMp3Header = std::make_shared<mp3Header>(mp3Header());
	//Shift to the right 12 bits and get 4 bits (0xF == 1111)
	pMp3Header->nBitrate = bitrate[(nHeaderBits >> 12) & 0xF];
	//Shift to the right 10 bits and get 2 bits (0x3 == 0011)
	pMp3Header->nSampleRate = freq[(nHeaderBits >> 10) & 0x3];
	//Check if the padding bit is set, which means to add 1 to the location of the next frame.
	//Shift to right 9 bits and get 1 bit (0x1 = 0001)
	unsigned int padding = (nHeaderBits >> 9) & 0x1;
	pMp3Header->spNextFrame = spCurrentPosition.operator+((144 * pMp3Header->nBitrate / pMp3Header->nSampleRate) + padding);
	return pMp3Header;
}

//Get the header at the given location
std::shared_ptr<mp3head::mp3Header> mp3head::getheaderat(std::streampos spPossibleHeader)
{
	fsInputMp3.seekg(spPossibleHeader);
	uint32_t nHeadBits;

	fsInputMp3.read(reinterpret_cast<char *>(&nHeadBits), 4);
	//Because of endianness, swap it the other way around.
	nHeadBits = _byteswap_ulong(nHeadBits);
	if (checkvalidheader(nHeadBits))
	{
		
		return getheader(nHeadBits, spPossibleHeader);
	}
	//Return nullptr as the failure value
	return nullptr;

}
//Find the next frame from a given position. Used to get the first frame of the mp3 file, but usually called multiple times.
std::streampos mp3head::findnextframe(std::streampos spSearchStart)
{
	
	fsInputMp3.seekg(spSearchStart);
	char cBuffer[4096];
	//Read the given file in chunks of 4kb
	while (fsInputMp3.read(cBuffer, sizeof cBuffer))
	{
		std::streamsize ssReadSize = fsInputMp3.gcount();
		std::streampos spPreviousPosition = fsInputMp3.tellg() - ssReadSize;
		//Go through each byte in the buffer and check if it is equal to two possible bytes
		for (int i = 0; i < ssReadSize; i++)
		{
			char cNibble1 = cBuffer[i];
			char cNibble2 = cBuffer[i + 1] >> 4;
			//
			if (cNibble1 == char(0xFF) && (cNibble2 == char(0xFF) || cNibble2 == char(0xFE)))
			{

				return spPreviousPosition.operator+(i);
			}
		}
	}
	//Return -1 if no next frame found.
	return -1;
}
//Find the first header in the given file.
std::shared_ptr<mp3head::mp3Header> mp3head::getfirstheader()
{
	std::streampos spHeaderSearchPosition = 0; //Start at start of file stream
	while (!fsInputMp3.eof()) //keep going until end of file reached
	{
		//Find the next valid frame from the current search position
		spHeaderSearchPosition = findnextframe(spHeaderSearchPosition);
		if (spHeaderSearchPosition.operator==(-1))
		{
			//Throw because if can't find even a single header, can't continue.
			throw std::exception("No valid mp3 headers");
		}
		//Turn the found header into a struct while also validating it, could put this straight into findnextframe
		std::shared_ptr<mp3Header> pHeader = getheaderat(spHeaderSearchPosition);
		if (pHeader != nullptr)
		{
			//If not nullptr, found a valid header.
			return pHeader;
		}
		spHeaderSearchPosition += 1;
	}
	//Throw because if can't find the first header, can't continue.
	throw std::exception("No valid mp3 headers");
}
//Gets all the headers starting from the first header and calculates the needed values.
void mp3head::getallheaders()
{
	//Get the first possible header, this method throws if none found.
	std::shared_ptr<mp3Header> firstHeader = getfirstheader();
	//Sample rate should be the same for each frame, so can assume it from the first frame.
	nSampleRate = firstHeader->nSampleRate;
	//Take the already calculated position of the next frame and set it to be checked next.
	std::streampos nextPos = firstHeader->spNextFrame;
	//Start calculating the average bitrate from first frame.
	unsigned long ulAvgBitrate = firstHeader->nBitrate;
	while (!fsInputMp3.eof())
	{

		auto pPossibleHeader = getheaderat(nextPos);
		if (pPossibleHeader == nullptr)
		{
			//If the next header is not found, it is usually the end of the mp3 file.
			break;
		}
		//Get the next position to check next loop.
		nextPos = pPossibleHeader->spNextFrame;
		//Add the bitrate to the current total value of bitrate to be averaged later
		ulAvgBitrate += pPossibleHeader->nBitrate;
		//Incremement the amount of frames
		nFrames++;
	}
	//Calculate the overall average.
	fAveBitRate = ulAvgBitrate / nFrames;
}
