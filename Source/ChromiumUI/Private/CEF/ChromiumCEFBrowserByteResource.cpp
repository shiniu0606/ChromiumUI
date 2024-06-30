// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEF/ChromiumCEFBrowserByteResource.h"

#if WITH_CEF3

FChromiumCEFBrowserByteResource::FChromiumCEFBrowserByteResource(const CefRefPtr<CefPostDataElement>& PostData, const FString& InMimeType)
	: Position(0)
	, Buffer(nullptr)
	, MimeType(InMimeType)
{
	Size = PostData->GetBytesCount();
	if (Size > 0)
	{
		Buffer = new unsigned char[Size];
		PostData->GetBytes(Size, Buffer);
	}
}

FChromiumCEFBrowserByteResource::~FChromiumCEFBrowserByteResource()
{
	delete[] Buffer;
}

void FChromiumCEFBrowserByteResource::Cancel()
{

}

void FChromiumCEFBrowserByteResource::GetResponseHeaders(CefRefPtr<CefResponse> Response, int64& ResponseLength, CefString& RedirectUrl)
{
	Response->SetMimeType(TCHAR_TO_WCHAR(*MimeType));
	Response->SetStatus(200);
	Response->SetStatusText("OK");
	ResponseLength = Size;
}

bool FChromiumCEFBrowserByteResource::ProcessRequest(CefRefPtr<CefRequest> Request, CefRefPtr<CefCallback> Callback)
{
	Callback->Continue();
	return true;
}

bool FChromiumCEFBrowserByteResource::ReadResponse(void* DataOut, int BytesToRead, int& BytesRead, CefRefPtr<CefCallback> Callback)
{
	int32 BytesLeft = Size - Position;
	BytesRead = BytesLeft >= BytesToRead ? BytesToRead : BytesLeft;
	if (BytesRead > 0)
	{
		FMemory::Memcpy(DataOut, Buffer + Position, BytesRead);
		Position += BytesRead;
		return true;
	}
	return false;
}
#endif
