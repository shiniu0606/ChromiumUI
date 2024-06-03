// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEF/ChromiumCEFBrowserApp.h"
#include "HAL/IConsoleManager.h"

#if WITH_CEF3

DEFINE_LOG_CATEGORY(ChromiumLogCEFBrowser);

static bool bCEFGPUAcceleration = true;
static FAutoConsoleVariableRef CVarCEFGPUAcceleration(
	TEXT("r.CEFGPUAcceleration"),
	bCEFGPUAcceleration,
	TEXT("Enables GPU acceleration in CEF\n"),
	ECVF_Default);

FChromiumCEFBrowserApp::FChromiumCEFBrowserApp()
	: MessagePumpCountdown(0)
{
}

void FChromiumCEFBrowserApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> CommandLine)
{
}

void FChromiumCEFBrowserApp::OnBeforeCommandLineProcessing(const CefString& ProcessType, CefRefPtr< CefCommandLine > CommandLine)
{
	if (bCEFGPUAcceleration)
	{
		UE_LOG(ChromiumLogCEFBrowser, Log, TEXT("CEF GPU acceleration enabled"));
		CommandLine->AppendSwitch("enable-gpu");
		CommandLine->AppendSwitch("enable-gpu-compositing");
	}
	else
	{
		UE_LOG(ChromiumLogCEFBrowser, Log, TEXT("CEF GPU acceleration disabled"));
		CommandLine->AppendSwitch("disable-gpu");
		CommandLine->AppendSwitch("disable-gpu-compositing");
	}
	CommandLine->AppendSwitch("enable-begin-frame-scheduling");
	CommandLine->AppendSwitch("disable-pinch"); // the web pages we have don't expect zoom to work right now so disable touchpad pinch zoom
	CommandLine->AppendSwitch("disable-gpu-shader-disk-cache"); // Don't create a "GPUCache" directory when cache-path is unspecified.
#if PLATFORM_MAC
	CommandLine->AppendSwitch("use-mock-keychain"); // Disable the toolchain prompt on macOS.
#endif
}

void FChromiumCEFBrowserApp::OnRenderProcessThreadCreated(CefRefPtr<CefListValue> ExtraInfo)
{
	RenderProcessThreadCreatedDelegate.ExecuteIfBound(ExtraInfo);
}

void FChromiumCEFBrowserApp::OnScheduleMessagePumpWork(int64 delay_ms)
{
	FScopeLock Lock(&MessagePumpCountdownCS);

	// As per CEF documentation, if delay_ms is <= 0, then the call to CefDoMessageLoopWork should happen reasonably soon.  If delay_ms is > 0, then the call
	//  to CefDoMessageLoopWork should be scheduled to happen after the specified delay and any currently pending scheduled call should be canceled.
	if(delay_ms < 0)
	{
		delay_ms = 0;
	}
	MessagePumpCountdown = delay_ms;
}

bool FChromiumCEFBrowserApp::TickMessagePump(float DeltaTime, bool bForce)
{
	bool bPump = false;
	{
		FScopeLock Lock(&MessagePumpCountdownCS);
		
		// count down in order to call message pump
		if (MessagePumpCountdown >= 0)
		{
			MessagePumpCountdown -= (DeltaTime * 1000);
			if (MessagePumpCountdown <= 0)
			{
				bPump = true;
			}
			
			if (bPump || bForce)
			{
				// -1 indicates that no countdown is currently happening
				MessagePumpCountdown = -1;
			}
		}
	}
	
	if (bPump || bForce)
	{
		CefDoMessageLoopWork();
		return true;
	}
	return false;
}

#endif
