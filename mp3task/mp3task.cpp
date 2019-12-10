/* some useful headers */
#include <stdio.h>
#include <errno.h>
#include <atlstr.h>
#include <windows.h>
#include <string>
#include <vector>
#include "mp3head.h"


/* this is the function we're looking to call, with 
   the values you have calculated */
void printmp3details(unsigned int nFrames, unsigned int nSampleRate, double fAveBitRate)
{
	printf("MP3 details:\n");
	printf("Frames: %d\n", nFrames);
	printf("Sample rate: %d\n", nSampleRate);
	printf("Ave nBitrate: %0.0f\n", fAveBitRate);
}

//Print the file details.
void printfiledetails(uint64_t nFileSize, DWORD dwFileAttr, std::string sCreationTime, std::string sLastAccessTime,
                      std::string sLastWriteTime)
{
	printf("File details:\n");
	printf("File size: %llu bytes\n", nFileSize);
	printf("Creation time: %s\n", sCreationTime.c_str());
	printf("Last access time: %s\n", sLastAccessTime.c_str());
	printf("last write time: %s\n", sLastWriteTime.c_str());
	printf("Read only: %s\n", dwFileAttr & FILE_ATTRIBUTE_READONLY ? "true" : "false"); //More readable format?
}

//Turn a filetime into a formatted string.
std::string getfiletimereadable(FILETIME tInputTime)
{
	SYSTEMTIME tOutputTime;
	char sOutputTimeString[25];
	FileTimeToSystemTime(&tInputTime, &tOutputTime);
	sprintf_s(sOutputTimeString, "%d/%d/%d %d:%d:%d:%d", tOutputTime.wDay, tOutputTime.wMonth, tOutputTime.wYear,
	          tOutputTime.wHour, tOutputTime.wMinute, tOutputTime.wSecond, tOutputTime.wMilliseconds);
	return sOutputTimeString;
}

/* Main function */
int main()
{
	//Using windows-only and thus non-portable libraries as this is a MSVC++ Project and it is unlikely to be compiled for other operating systems.
	//The libraries give a lot of convenient access to the file attributes, that other C++ methods don't supply as easily.
	int nArgs;
	//Use this method instead of usual arg retrieval to get args as LPWSTR for later use
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (nArgs <= 1)
	{
		printf("No file argument supplied, please supply a file argument\nBitrate");
	}

	WIN32_FILE_ATTRIBUTE_DATA fAttData;
	//got 1 argument, assume it is file
	if (!GetFileAttributesEx(szArglist[1], GetFileExInfoStandard, &fAttData)) //Get the attribrutes of the given file.
	{
		return 1;
	}
	ULARGE_INTEGER nFileSize; //move all file stuff to another function
	nFileSize.HighPart = fAttData.nFileSizeHigh;
	nFileSize.LowPart = fAttData.nFileSizeLow;

	//Convert first argument to string to be more easily used;
	std::string sFilename = CW2A(szArglist[1]);

	mp3head mp3FileParsed = mp3head(sFilename);
	printfiledetails(nFileSize.QuadPart, fAttData.dwFileAttributes,
	                 getfiletimereadable(fAttData.ftCreationTime), getfiletimereadable(fAttData.ftLastAccessTime),
	                 getfiletimereadable(fAttData.ftLastWriteTime));
	printmp3details(mp3FileParsed.getframecount(), mp3FileParsed.getsamplerate(), mp3FileParsed.getavgbitrate());

	//Free up szArgList pointer
	LocalFree(szArglist);
	return 0;
}
