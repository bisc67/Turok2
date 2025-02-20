#include "AddEntityOp.h"
#include "WONCommon/WriteBuffer.h"

using namespace WONAPI;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void AddEntityOp::Init()
{
	mFlags = DIR_UF_UNIQUEDISPLAYNAME;
	mLengthFieldSize = 4;
	mLifespan = 0;
	mIsService = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
AddEntityOp::AddEntityOp(ServerContext *theDirContext, bool isService) : ServerRequestOp(theDirContext)
{
	Init();
	mIsService = isService;
}
   
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
AddEntityOp::AddEntityOp(const IPAddr &theAddr, bool isService) : ServerRequestOp(theAddr)
{
	Init();
	mIsService = isService;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
WONStatus AddEntityOp::GetNextRequest()
{
	WriteBuffer aMsg(mLengthFieldSize);
	aMsg.AppendByte(5);						// Small message
	aMsg.AppendShort(2);					// Directory Server
	aMsg.AppendShort(mIsService?215:214);	// AddServiceEx : AddDirectoryEx
	aMsg.AppendByte(mFlags);				// Entity Flags
	aMsg.AppendWString(mPath);				// Directory path
	aMsg.AppendWString(mName);				// Entity name

	if(mIsService)
		aMsg.AppendBuffer(mNetAddr,1);

	aMsg.AppendWString(mDisplayName);		// Entity display name
	aMsg.AppendLong(mLifespan);				// Entity lifespan
	aMsg.AppendShort(mDataObjects.size());	// num data objects
	
	DirDataObjectList::iterator anItr = mDataObjects.begin();
	while(anItr!=mDataObjects.end())
	{
		DirDataObject &aDataObject = *anItr;
		aMsg.AppendString(aDataObject.mDataType,1);
		aMsg.AppendBuffer(aDataObject.mData,2);
		++anItr;
	}

	aMsg.AppendShort(0); // num permissions
	mRequest = aMsg.ToByteBuffer();
	return WS_ServerReq_Recv;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
WONStatus AddEntityOp::CheckResponse()
{
	ReadBuffer aMsg(mResponse->data(),mResponse->length());
	unsigned char aHeaderType = aMsg.ReadByte();
	unsigned short aServiceType = aMsg.ReadShort();
	unsigned short aMessageType = aMsg.ReadShort();

	if(aHeaderType!=5 || aServiceType!=2 || aMessageType!=1)
		return InvalidReplyHeader();


	short aStatus = aMsg.ReadShort();
	if(aStatus>=0 && mIsService)
	{
		if((mFlags & DIR_UF_SERVRETURNADDR)!=0)
	{
			unsigned char aLen = aMsg.ReadByte();
			mNetAddr = new ByteBuffer(aMsg.ReadBytes(aLen),aLen);
		}
	}

	return (WONStatus)aStatus;
}

