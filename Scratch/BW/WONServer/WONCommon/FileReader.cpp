#include "FileReader.h"
#include "LittleEndian.h"

using namespace WONAPI;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
FileReader::FileReader()
{
	mFile = NULL;
	mMemDataP = NULL;
	mMemDataLen = 0;
	mMemDataPos = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
FileReader::~FileReader()
{
	if(mFile!=NULL)
		fclose(mFile);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool FileReader::Open(const char *theFilePath)
{
	Close();
	mFile = fopen(theFilePath,"rb");
	return mFile!=NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void FileReader::Open(const void *theMemDataP, unsigned long theMemDataLen)
{
	Close();
	mMemDataP = (const char*)theMemDataP;
	mMemDataLen = theMemDataLen;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void FileReader::Close()
{
	mMemDataP = NULL;
	mMemDataLen = 0;
	mMemDataPos = 0;

	if(mFile!=NULL)
	{
		fclose(mFile);
		mFile = NULL;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void FileReader::ReadBytes(void *theBuf, unsigned long theNumBytes)
{
	if(mMemDataP!=NULL)
	{
		if(mMemDataPos + theNumBytes > mMemDataLen)
			throw FileReaderException("Attempt to read past end of mem buffer.");

		memcpy(theBuf, mMemDataP + mMemDataPos, theNumBytes);
		mMemDataPos += theNumBytes;
	}
	else
	{
		if(mFile==NULL)
			throw FileReaderException("NULL File.");

		if(fread(theBuf,1,theNumBytes,mFile)!=theNumBytes)
			throw FileReaderException("Attempt to read past end of file.");
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void FileReader::SkipBytes(unsigned long theNumBytes)
{
	if(mMemDataP!=NULL)
	{
		if(mMemDataPos + theNumBytes > mMemDataLen)
			throw FileReaderException("Attempt to read past end of mem buffer.");

		mMemDataPos += theNumBytes;
	}
	else
	{
		if(mFile==NULL)
			throw FileReaderException("NULL File.");

		if(fseek(mFile,theNumBytes,SEEK_CUR)!=0)
			throw FileReaderException("Attempt to read past end of file.");
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
unsigned char FileReader::ReadByte()
{
	unsigned char aByte;
	ReadBytes(&aByte,1);
	return aByte;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
unsigned short FileReader::ReadShort()
{
	unsigned short aShort;
	ReadBytes(&aShort,2);
	return ShortFromLittleEndian(aShort);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
unsigned long FileReader::ReadLong()
{
	unsigned long aLong;
	ReadBytes(&aLong,4);
	return LongFromLittleEndian(aLong);
}
