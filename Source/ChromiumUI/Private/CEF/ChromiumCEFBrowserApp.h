// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"

#if WITH_CEF3

#include "ChromiumCEFLibCefIncludes.h"

DECLARE_LOG_CATEGORY_EXTERN(ChromiumLogCEFBrowser, Log, All);

/**
 * Implements CEF App and other Process level interfaces
 */
class FChromiumCEFBrowserApp : public CefApp,
	public CefBrowserProcessHandler
{
public:

	/**
	 * Default Constructor
	 */
	FChromiumCEFBrowserApp();

	/** A delegate this is invoked when an existing browser requests creation of a new browser window. */
	DECLARE_DELEGATE_OneParam(FOnRenderProcessThreadCreated, CefRefPtr<CefListValue> /*ExtraInfo*/);
	virtual FOnRenderProcessThreadCreated& OnRenderProcessThreadCreated()
	{
		return RenderProcessThreadCreatedDelegate;
	}

	/** Used to pump the CEF message loop whenever OnScheduleMessagePumpWork is triggered */
	bool TickMessagePump(float DeltaTime, bool bForce);

private:
	// CefApp methods.
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
	virtual void OnBeforeCommandLineProcessing(const CefString& ProcessType, CefRefPtr< CefCommandLine > CommandLine) override;
	// CefBrowserProcessHandler methods:
	virtual void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> CommandLine) override;
	virtual void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> ExtraInfo) override;
	virtual void OnScheduleMessagePumpWork(int64 delay_ms) override;

	FOnRenderProcessThreadCreated RenderProcessThreadCreatedDelegate;

	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(FChromiumCEFBrowserApp);

	// Lock for access MessagePumpCountdown
	FCriticalSection MessagePumpCountdownCS;
	// Countdown in milliseconds until CefDoMessageLoopWork is called.  Updated by OnScheduleMessagePumpWork
	int64 MessagePumpCountdown;
};
#endif
