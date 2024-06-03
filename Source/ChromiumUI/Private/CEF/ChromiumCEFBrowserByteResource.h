// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_CEF3


#include "ChromiumCEFLibCefIncludes.h"



/**
 * Implements a resource handler that will return the contents of a string as the result.
 */
class FChromiumCEFBrowserByteResource
	: public CefResourceHandler
{
public:
	/**
	 */
	FChromiumCEFBrowserByteResource(const CefRefPtr<CefPostDataElement>& PostData, const FString& InMimeType);
	~FChromiumCEFBrowserByteResource();
	
	// CefResourceHandler interface
	virtual void Cancel() override;
	virtual void GetResponseHeaders(CefRefPtr<CefResponse> Response, int64& ResponseLength, CefString& RedirectUrl) override;
	virtual bool ProcessRequest(CefRefPtr<CefRequest> Request, CefRefPtr<CefCallback> Callback) override;
	virtual bool ReadResponse(void* DataOut, int BytesToRead, int& BytesRead, CefRefPtr<CefCallback> Callback) override;
	
private:
	int32 Position;
	int32 Size;
	unsigned char* Buffer;
	FString MimeType;
	
	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(FChromiumCEFBrowserByteResource);
};


#endif
