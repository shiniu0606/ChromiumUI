// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_CEF3

#include "IChromiumWebBrowserResourceLoader.h"

#include "ChromiumCEFLibCefIncludes.h"


FString ChromiumResourceTypeToString(const CefRequest::ResourceType& Type);



/**
 * Implements CEF Request handling for when a browser window is still being constructed
 */
class FChromiumCEFResourceContextHandler :
	  public CefRequestContextHandler
	, public CefResourceRequestHandler
{
public:

	/** Default constructor. */
	FChromiumCEFResourceContextHandler();

public:

	// CefResourceRequestHandler Interface
	virtual CefResourceRequestHandler::ReturnValue OnBeforeResourceLoad(
		CefRefPtr<CefBrowser> Browser,
		CefRefPtr<CefFrame> Frame,
		CefRefPtr<CefRequest> Request,
		CefRefPtr<CefRequestCallback> Callback) override;

	// CefRequestContextHandler Interface
	virtual CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		bool is_navigation,
		bool is_download,
		const CefString& request_initiator,
		bool& disable_default_handling) override;


public:
	FChromiumOnBeforeContextResourceLoadDelegate& OnBeforeLoad()
	{
		return BeforeResourceLoadDelegate;
	}

private:

	/** Delegate for handling resource load requests */
	FChromiumOnBeforeContextResourceLoadDelegate BeforeResourceLoadDelegate;

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(FChromiumCEFResourceContextHandler);
};

#endif
