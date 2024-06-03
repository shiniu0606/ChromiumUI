// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEF/ChromiumCEFBrowserHandler.h"
#include "HAL/PlatformApplicationMisc.h"

#if WITH_CEF3

//#define DEBUG_ONBEFORELOAD // Debug print beforebrowse steps, used in CEFBrowserHandler.h so define early 

#include "ChromiumWebBrowserModule.h"
#include "ChromiumCEFBrowserClosureTask.h"
#include "IChromiumWebBrowserSingleton.h"
#include "ChromiumWebBrowserSingleton.h"
#include "ChromiumCEFBrowserPopupFeatures.h"
#include "ChromiumCEFWebBrowserWindow.h"
#include "ChromiumCEFBrowserByteResource.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/ThreadingBase.h"
#include "PlatformHttp.h"
#include "Misc/CommandLine.h"

#define LOCTEXT_NAMESPACE "ChromiumWebBrowserHandler"


// Used to force returning custom content instead of performing a request.
const FString CustomContentMethod(TEXT("X-GET-CUSTOM-CONTENT"));

FChromiumCEFBrowserHandler::FChromiumCEFBrowserHandler(bool InUseTransparency, const TArray<FString>& InAltRetryDomains)
: bUseTransparency(InUseTransparency), 
bAllowAllCookies(false),
AltRetryDomains(InAltRetryDomains)
{
	// should we forcefully allow all cookies to be set rather than filtering a couple store side ones
	bAllowAllCookies = FParse::Param(FCommandLine::Get(), TEXT("CefAllowAllCookies"));
}

void FChromiumCEFBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> Browser, const CefString& Title)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetTitle(Title);
	}
}

void FChromiumCEFBrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, const CefString& Url)
{
	if (Frame->IsMain())
	{
		TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

		if (BrowserWindow.IsValid())
		{
			BrowserWindow->SetUrl(Url);
		}
	}
}

bool FChromiumCEFBrowserHandler::OnTooltip(CefRefPtr<CefBrowser> Browser, CefString& Text)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetToolTip(Text);
	}

	return false;
}

bool FChromiumCEFBrowserHandler::OnConsoleMessage(CefRefPtr<CefBrowser> Browser, cef_log_severity_t level, const CefString& Message, const CefString& Source, int Line)
{
	ConsoleMessageDelegate.ExecuteIfBound(Browser, level, Message, Source, Line);
	// Return false to let it output to console.
	return false;
}

void FChromiumCEFBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> Browser)
{
	if(Browser->IsPopup())
	{
		TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindowParent = ParentHandler.get() ? ParentHandler->BrowserWindowPtr.Pin() : nullptr;
		if(BrowserWindowParent.IsValid() && ParentHandler->OnCreateWindow().IsBound())
		{
			TSharedPtr<FChromiumWebBrowserWindowInfo> NewBrowserWindowInfo = MakeShareable(new FChromiumWebBrowserWindowInfo(Browser, this));
			TSharedPtr<IChromiumWebBrowserWindow> NewBrowserWindow = IChromiumWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow(
				BrowserWindowParent,
				NewBrowserWindowInfo
				);

			{
				// @todo: At the moment we need to downcast since the handler does not support using the interface.
				TSharedPtr<FChromiumCEFWebBrowserWindow> HandlerSpecificBrowserWindow = StaticCastSharedPtr<FChromiumCEFWebBrowserWindow>(NewBrowserWindow);
				BrowserWindowPtr = HandlerSpecificBrowserWindow;
			}

			// Request a UI window for the browser.  If it is not created we do some cleanup.
			bool bUIWindowCreated = ParentHandler->OnCreateWindow().Execute(TWeakPtr<IChromiumWebBrowserWindow>(NewBrowserWindow), TWeakPtr<IChromiumWebBrowserPopupFeatures>(BrowserPopupFeatures));
			if(!bUIWindowCreated)
			{
				NewBrowserWindow->CloseBrowser(true);
			}
			else
			{
				checkf(!NewBrowserWindow.IsUnique(), TEXT("Handler indicated that new window UI was created, but failed to save the new WebBrowserWindow instance."));
			}
		}
		else
		{
			Browser->GetHost()->CloseBrowser(true);
		}
	}
}

bool FChromiumCEFBrowserHandler::DoClose(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if(BrowserWindow.IsValid())
	{
		BrowserWindow->OnBrowserClosing();
	}
#if PLATFORM_WINDOWS
	// If we have a window handle, we're rendering directly to the screen and not off-screen
	HWND NativeWindowHandle = Browser->GetHost()->GetWindowHandle();
	if (NativeWindowHandle != nullptr)
	{
		HWND ParentWindow = ::GetParent(NativeWindowHandle);

		if (ParentWindow)
		{
			HWND FocusHandle = ::GetFocus();
			if (FocusHandle && (FocusHandle == NativeWindowHandle || ::IsChild(NativeWindowHandle, FocusHandle)))
			{
				// Set focus to the parent window, otherwise keyboard and mouse wheel input will become wonky
				::SetFocus(ParentWindow);
			}
			// CEF will send a WM_CLOSE to the parent window and potentially exit the application if we don't do this
			::SetParent(NativeWindowHandle, nullptr);
		}
	}
#endif
	return false;
}

void FChromiumCEFBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnBrowserClosed();
	}

}

bool FChromiumCEFBrowserHandler::OnBeforePopup( CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	const CefString& TargetUrl,
	const CefString& TargetFrameName,
	const CefPopupFeatures& PopupFeatures,
	CefWindowInfo& OutWindowInfo,
	CefRefPtr<CefClient>& OutClient,
	CefBrowserSettings& OutSettings,
	bool* OutNoJavascriptAccess )
{
	FString URL = WCHAR_TO_TCHAR(TargetUrl.ToWString().c_str());
	FString FrameName = WCHAR_TO_TCHAR(TargetFrameName.ToWString().c_str());

	/* If OnBeforePopup() is not bound, we allow creating new windows as long as OnCreateWindow() is bound.
	   The BeforePopup delegate is always executed even if OnCreateWindow is not bound to anything .
	  */
	if((OnBeforePopup().IsBound() && OnBeforePopup().Execute(URL, FrameName)) || !OnCreateWindow().IsBound())
	{
		return true;
	}
	else
	{
		TSharedPtr<FChromiumCEFBrowserPopupFeatures> NewBrowserPopupFeatures = MakeShareable(new FChromiumCEFBrowserPopupFeatures(PopupFeatures));
		bool bIsDevtools = URL.Contains(TEXT("chrome-devtools"));
		bool shouldUseTransparency = bIsDevtools ? false : bUseTransparency;
		NewBrowserPopupFeatures->SetResizable(bIsDevtools); // only have the window for DevTools have resize options

		cef_color_t Alpha = shouldUseTransparency ? 0 : CefColorGetA(OutSettings.background_color);
		cef_color_t R = CefColorGetR(OutSettings.background_color);
		cef_color_t G = CefColorGetG(OutSettings.background_color);
		cef_color_t B = CefColorGetB(OutSettings.background_color);
		OutSettings.background_color = CefColorSetARGB(Alpha, R, G, B);

		CefRefPtr<FChromiumCEFBrowserHandler> NewHandler(new FChromiumCEFBrowserHandler(shouldUseTransparency));
		NewHandler->ParentHandler = this;
		NewHandler->SetPopupFeatures(NewBrowserPopupFeatures);
		OutClient = NewHandler;

		// Always use off screen rendering so we can integrate with our windows
		// Always use off screen rendering so we can integrate with our windows
#if PLATFORM_LINUX
		OutWindowInfo.SetAsWindowless(kNullWindowHandle, shouldUseTransparency);
#else
		OutWindowInfo.SetAsWindowless(kNullWindowHandle);
#endif

		// We need to rely on CEF to create our window so we set the WindowInfo, BrowserSettings, Client, and then return false
		return false;
	}
}

bool FChromiumCEFBrowserHandler::OnCertificateError(CefRefPtr<CefBrowser> Browser,
	cef_errorcode_t CertError,
	const CefString &RequestUrl,
	CefRefPtr<CefSSLInfo> SslInfo,
	CefRefPtr<CefRequestCallback> Callback)
{
	// Forward the cert error to the normal load error handler
	CefString ErrorText = "Certificate error";
	OnLoadError(Browser, Browser->GetMainFrame(), CertError, ErrorText, RequestUrl);
	return false;
}

void FChromiumCEFBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	CefLoadHandler::ErrorCode InErrorCode,
	const CefString& ErrorText,
	const CefString& FailedUrl)
{

	// notify browser window
	if (Frame->IsMain())
	{
		TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

		if (BrowserWindow.IsValid())
		{
			if (AltRetryDomains.Num() > 0 && AltRetryDomainIdx < (uint32)AltRetryDomains.Num())
			{
				FString Url = WCHAR_TO_TCHAR(FailedUrl.ToWString().c_str());
				FString OriginalUrlDomain = FPlatformHttp::GetUrlDomain(Url);
				if (!OriginalUrlDomain.IsEmpty())
				{
					const FString NewUrl(Url.Replace(*OriginalUrlDomain, *AltRetryDomains[AltRetryDomainIdx++]));
					BrowserWindow->LoadURL(NewUrl);
					return;
				}

			}
			BrowserWindow->NotifyDocumentError(InErrorCode, ErrorText, FailedUrl);
		}
	}
}

void FChromiumCEFBrowserHandler::OnLoadStart(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, TransitionType CefTransitionType)
{
}

void FChromiumCEFBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> Browser, bool bIsLoading, bool bCanGoBack, bool bCanGoForward)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->NotifyDocumentLoadingStateChange(bIsLoading);
	}
}

bool FChromiumCEFBrowserHandler::GetRootScreenRect(CefRefPtr<CefBrowser> Browser, CefRect& Rect)
{
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);
	Rect.width = DisplayMetrics.PrimaryDisplayWidth;
	Rect.height = DisplayMetrics.PrimaryDisplayHeight;
	return true;
}

void FChromiumCEFBrowserHandler::GetViewRect(CefRefPtr<CefBrowser> Browser, CefRect& Rect)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->GetViewRect(Rect);
	}
	else
	{
		// CEF requires at least a 1x1 area for painting
		Rect.x = Rect.y = 0;
		Rect.width = Rect.height = 1;
	}
}

void FChromiumCEFBrowserHandler::OnPaint(CefRefPtr<CefBrowser> Browser,
	PaintElementType Type,
	const RectList& DirtyRects,
	const void* Buffer,
	int Width, int Height)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnPaint(Type, DirtyRects, Buffer, Width, Height);
	}
}

void FChromiumCEFBrowserHandler::OnAcceleratedPaint(CefRefPtr<CefBrowser> Browser,
	PaintElementType Type,
	const RectList& DirtyRects,
	void* SharedHandle)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnAcceleratedPaint(Type, DirtyRects, SharedHandle);
	}
}

void FChromiumCEFBrowserHandler::OnCursorChange(CefRefPtr<CefBrowser> Browser, CefCursorHandle Cursor, CefRenderHandler::CursorType Type, const CefCursorInfo& CustomCursorInfo)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnCursorChange(Cursor, Type, CustomCursorInfo);
	}

}

void FChromiumCEFBrowserHandler::OnPopupShow(CefRefPtr<CefBrowser> Browser, bool bShow)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->ShowPopupMenu(bShow);
	}

}

void FChromiumCEFBrowserHandler::OnPopupSize(CefRefPtr<CefBrowser> Browser, const CefRect& Rect)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetPopupMenuPosition(Rect);
	}
}

bool FChromiumCEFBrowserHandler::GetScreenInfo(CefRefPtr<CefBrowser> Browser, CefScreenInfo& ScreenInfo)
{
	TSharedPtr<FChromiumWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	ScreenInfo.depth = 24;

	if (BrowserWindow.IsValid() && BrowserWindow->GetParentWindow().IsValid())
	{
		ScreenInfo.device_scale_factor = BrowserWindow->GetParentWindow()->GetNativeWindow()->GetDPIScaleFactor();
	}
	else
	{
		FDisplayMetrics DisplayMetrics;
		FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);
		ScreenInfo.device_scale_factor = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayMetrics.PrimaryDisplayWorkAreaRect.Left, DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);
	}
	return true;
}


#if !PLATFORM_LINUX
void FChromiumCEFBrowserHandler::OnImeCompositionRangeChanged(
	CefRefPtr<CefBrowser> Browser,
	const CefRange& SelectionRange,
	const CefRenderHandler::RectList& CharacterBounds)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnImeCompositionRangeChanged(Browser, SelectionRange, CharacterBounds);
	}
}
#endif


CefResourceRequestHandler::ReturnValue FChromiumCEFBrowserHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, CefRefPtr<CefRequest> Request, CefRefPtr<CefRequestCallback> Callback)
{
	// Current thread is IO thread. We need to invoke BrowserWindow->GetResourceContent on the UI (aka Game) thread:
	CefPostTask(TID_UI, new FChromiumCEFBrowserClosureTask(this, [=]()
	{
		const FString LanguageHeaderText(TEXT("Accept-Language"));
		const FString LocaleCode = FChromiumWebBrowserSingleton::GetCurrentLocaleCode();
		CefRequest::HeaderMap HeaderMap;
		Request->GetHeaderMap(HeaderMap);
		auto LanguageHeader = HeaderMap.find(TCHAR_TO_WCHAR(*LanguageHeaderText));
		if (LanguageHeader != HeaderMap.end())
		{
			(*LanguageHeader).second = TCHAR_TO_WCHAR(*LocaleCode);
		}
		else
		{
			HeaderMap.insert(std::pair<CefString, CefString>(TCHAR_TO_WCHAR(*LanguageHeaderText), TCHAR_TO_WCHAR(*LocaleCode)));
		}
		
#ifdef DEBUG_ONBEFORELOAD
		auto url = Request->GetURL();
		auto type = Request->GetResourceType();
		if (type == CefRequest::ResourceType::RT_MAIN_FRAME || type == CefRequest::ResourceType::RT_XHR)
		{
			GLog->Logf(ELogVerbosity::Display, TEXT("FChromiumCEFBrowserHandler::OnBeforeResourceLoad :%s"), url.c_str());
		}
#endif

		if (BeforeResourceLoadDelegate.IsBound())
		{
			FRequestHeaders AdditionalHeaders;
			BeforeResourceLoadDelegate.Execute(Request->GetURL(), Request->GetResourceType(), AdditionalHeaders);

			for (auto Iter = AdditionalHeaders.CreateConstIterator(); Iter; ++Iter)
			{
				const FString& Header = Iter.Key();
				const FString& Value = Iter.Value();

				HeaderMap.insert(std::pair<CefString, CefString>(TCHAR_TO_WCHAR(*Header), TCHAR_TO_WCHAR(*Value)));
			}
		}

		TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

		if (BrowserWindow.IsValid())
		{
			TOptional<FString> Contents = BrowserWindow->GetResourceContent(Frame, Request);
			if(Contents.IsSet())
			{
				Contents.GetValue().ReplaceInline(TEXT("\n"), TEXT(""), ESearchCase::CaseSensitive);
				Contents.GetValue().ReplaceInline(TEXT("\r"), TEXT(""), ESearchCase::CaseSensitive);

				// pass the text we'd like to come back as a response to the request post data
				CefRefPtr<CefPostData> PostData = CefPostData::Create();
				CefRefPtr<CefPostDataElement> Element = CefPostDataElement::Create();
				FTCHARToUTF8 UTF8String(*Contents.GetValue());
				Element->SetToBytes(UTF8String.Length(), UTF8String.Get());
				PostData->AddElement(Element);
				Request->SetPostData(PostData);

				// Set a custom request header, so we know the mime type if it was specified as a hash on the dummy URL
				std::string Url = Request->GetURL().ToString();
				std::string::size_type HashPos = Url.find_last_of('#');
				if (HashPos != std::string::npos)
				{
					std::string MimeType = Url.substr(HashPos + 1);
					HeaderMap.insert(std::pair<CefString, CefString>(TCHAR_TO_WCHAR(TEXT("Content-Type")), MimeType));
				}

				// Change http method to tell GetResourceHandler to return the content
				Request->SetMethod(TCHAR_TO_WCHAR(*CustomContentMethod));
			}
		}

		Request->SetHeaderMap(HeaderMap);

		Callback->Continue(true);
	}));

	// Tell CEF that we're handling this asynchronously.
	return RV_CONTINUE_ASYNC;
}

void FChromiumCEFBrowserHandler::OnResourceLoadComplete(
	CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	CefRefPtr<CefRequest> Request,
	CefRefPtr<CefResponse> Response,
	URLRequestStatus Status,
	int64 Received_content_length)
{
	// Current thread is IO thread. We need to invoke our delegates on the UI (aka Game) thread:
	CefPostTask(TID_UI, new FChromiumCEFBrowserClosureTask(this, [=]()
	{
		ResourceLoadCompleteDelegate.ExecuteIfBound(Request->GetURL(), Request->GetResourceType(), Status, Received_content_length);
	}));
}

void FChromiumCEFBrowserHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> Browser, TerminationStatus Status)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnRenderProcessTerminated(Status);
	}
}

bool FChromiumCEFBrowserHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	CefRefPtr<CefRequest> Request,
	bool user_gesture, 
	bool IsRedirect)
{
	// Current thread: UI thread
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
#ifdef DEBUG_ONBEFORELOAD
		auto url = Request->GetURL();
		auto type = Request->GetResourceType();
		if (type == CefRequest::ResourceType::RT_MAIN_FRAME || type == CefRequest::ResourceType::RT_XHR)
		{
			GLog->Logf(ELogVerbosity::Display, TEXT("FChromiumCEFBrowserHandler::OnBeforeBrowse :%s"), url.c_str());
		}
#endif
		if(BrowserWindow->OnBeforeBrowse(Browser, Frame, Request, user_gesture, IsRedirect))
		{
			return true;
		}
	}

	return false;
}

CefRefPtr<CefResourceHandler> FChromiumCEFBrowserHandler::GetResourceHandler( CefRefPtr<CefBrowser> Browser, CefRefPtr< CefFrame > Frame, CefRefPtr< CefRequest > Request )
{

	if (Request->GetMethod() == TCHAR_TO_WCHAR(*CustomContentMethod))
	{
		// Content override header will be set by OnBeforeResourceLoad before passing the request on to this.
		if (Request->GetPostData() && Request->GetPostData()->GetElementCount() > 0)
		{
			// get the mime type from Content-Type header (default to text/html to support old behavior)
			FString MimeType = TEXT("text/html"); // default if not specified
			CefRequest::HeaderMap HeaderMap;
			Request->GetHeaderMap(HeaderMap);
			auto ContentOverride = HeaderMap.find(TCHAR_TO_WCHAR(TEXT("Content-Type")));
			if (ContentOverride != HeaderMap.end())
			{
				MimeType = WCHAR_TO_TCHAR(ContentOverride->second.ToWString().c_str());
			}

			// reply with the post data
			CefPostData::ElementVector Elements;
			Request->GetPostData()->GetElements(Elements);
			return new FChromiumCEFBrowserByteResource(Elements[0], MimeType);
		}
	}
	return nullptr;
}

CefRefPtr<CefResourceRequestHandler> FChromiumCEFBrowserHandler::GetResourceRequestHandler( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
	CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling) 
{
#ifdef DEBUG_ONBEFORELOAD
	auto url = request->GetURL();
	auto type = request->GetResourceType();
	if (type == CefRequest::ResourceType::RT_MAIN_FRAME || type == CefRequest::ResourceType::RT_XHR)
	{
		GLog->Logf(ELogVerbosity::Display, TEXT("FChromiumCEFBrowserHandler::GetResourceRequestHandler :%s"), url.c_str());
	}
#endif
	return this;
}

void FChromiumCEFBrowserHandler::SetBrowserWindow(TSharedPtr<FChromiumCEFWebBrowserWindow> InBrowserWindow)
{
	BrowserWindowPtr = InBrowserWindow;
}

bool FChromiumCEFBrowserHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> frame,
	CefProcessId SourceProcess,
	CefRefPtr<CefProcessMessage> Message)
{
	bool Retval = false;
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		Retval = BrowserWindow->OnProcessMessageReceived(Browser, frame, SourceProcess, Message);
	}
	return Retval;
}

bool FChromiumCEFBrowserHandler::ShowDevTools(const CefRefPtr<CefBrowser>& Browser)
{
	CefPoint Point;
	CefString TargetUrl = "chrome-devtools://devtools/devtools.html";
	CefString TargetFrameName = "devtools";
	CefPopupFeatures PopupFeatures;
	CefWindowInfo WindowInfo;
	CefRefPtr<CefClient> NewClient;
	CefBrowserSettings BrowserSettings;
	bool NoJavascriptAccess = false;

	PopupFeatures.xSet = false;
	PopupFeatures.ySet = false;
	PopupFeatures.heightSet = false;
	PopupFeatures.widthSet = false;
	PopupFeatures.menuBarVisible = false;
	PopupFeatures.toolBarVisible  = false;
	PopupFeatures.statusBarVisible  = false;

	// Set max framerate to maximum supported.
	BrowserSettings.windowless_frame_rate = 60;
	// Disable plugins
	BrowserSettings.plugins = STATE_DISABLED;
	// Dev Tools look best with a white background color
	BrowserSettings.background_color = CefColorSetARGB(255, 255, 255, 255);

	// OnBeforePopup already takes care of all the details required to ask the host application to create a new browser window.
	bool bSuppressWindowCreation = OnBeforePopup(Browser, Browser->GetFocusedFrame(), TargetUrl, TargetFrameName, PopupFeatures, WindowInfo, NewClient, BrowserSettings, &NoJavascriptAccess);

	if(! bSuppressWindowCreation)
	{
		Browser->GetHost()->ShowDevTools(WindowInfo, NewClient, BrowserSettings, Point);
	}
	return !bSuppressWindowCreation;
}

bool FChromiumCEFBrowserHandler::OnKeyEvent(CefRefPtr<CefBrowser> Browser,
	const CefKeyEvent& Event,
	CefEventHandle OsEvent)
{
	// Show dev tools on CMD/CTRL+SHIFT+I
	if( (Event.type == KEYEVENT_RAWKEYDOWN || Event.type == KEYEVENT_KEYDOWN || Event.type == KEYEVENT_CHAR) &&
#if PLATFORM_MAC
		(Event.modifiers == (EVENTFLAG_COMMAND_DOWN | EVENTFLAG_SHIFT_DOWN)) &&
#else
		(Event.modifiers == (EVENTFLAG_CONTROL_DOWN | EVENTFLAG_SHIFT_DOWN)) &&
#endif
		(Event.windows_key_code == 'I' ||
		Event.unmodified_character == 'i' || Event.unmodified_character == 'I') &&
		IChromiumWebBrowserModule::Get().GetSingleton()->IsDevToolsShortcutEnabled()
	  )
	{
		return ShowDevTools(Browser);
	}

#if PLATFORM_MAC
	// We need to handle standard Copy/Paste/etc... shortcuts on OS X
	if( (Event.type == KEYEVENT_RAWKEYDOWN || Event.type == KEYEVENT_KEYDOWN) &&
		(Event.modifiers & EVENTFLAG_COMMAND_DOWN) != 0 &&
		(Event.modifiers & EVENTFLAG_CONTROL_DOWN) == 0 &&
		(Event.modifiers & EVENTFLAG_ALT_DOWN) == 0 &&
		( (Event.modifiers & EVENTFLAG_SHIFT_DOWN) == 0 || Event.unmodified_character == 'z' )
	  )
	{
		CefRefPtr<CefFrame> Frame = Browser->GetFocusedFrame();
		if (Frame)
		{
			switch (Event.unmodified_character)
			{
				case 'a':
					Frame->SelectAll();
					return true;
				case 'c':
					Frame->Copy();
					return true;
				case 'v':
					Frame->Paste();
					return true;
				case 'x':
					Frame->Cut();
					return true;
				case 'z':
					if( (Event.modifiers & EVENTFLAG_SHIFT_DOWN) == 0 )
					{
						Frame->Undo();
					}
					else
					{
						Frame->Redo();
					}
					return true;
			}
		}
	}
#endif
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->OnUnhandledKeyEvent(Event);
	}

	return false;
}

bool FChromiumCEFBrowserHandler::OnJSDialog(CefRefPtr<CefBrowser> Browser, const CefString& OriginUrl, JSDialogType DialogType, const CefString& MessageText, const CefString& DefaultPromptText, CefRefPtr<CefJSDialogCallback> Callback, bool& OutSuppressMessage)
{
	bool Retval = false;
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		Retval = BrowserWindow->OnJSDialog(DialogType, MessageText, DefaultPromptText, Callback, OutSuppressMessage);
	}
	return Retval;
}

bool FChromiumCEFBrowserHandler::OnBeforeUnloadDialog(CefRefPtr<CefBrowser> Browser, const CefString& MessageText, bool IsReload, CefRefPtr<CefJSDialogCallback> Callback)
{
	bool Retval = false;
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		Retval = BrowserWindow->OnBeforeUnloadDialog(MessageText, IsReload, Callback);
	}
	return Retval;
}

void FChromiumCEFBrowserHandler::OnResetDialogState(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnResetDialogState();
	}
}

void FChromiumCEFBrowserHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, CefRefPtr<CefContextMenuParams> Params, CefRefPtr<CefMenuModel> Model)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if ( BrowserWindow.IsValid() && BrowserWindow->OnSuppressContextMenu().IsBound() && BrowserWindow->OnSuppressContextMenu().Execute() )
	{
		Model->Clear();
	}
}

void FChromiumCEFBrowserHandler::OnDraggableRegionsChanged(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> frame, const std::vector<CefDraggableRegion>& Regions)
{
	TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		TArray<FChromiumWebBrowserDragRegion> DragRegions;
		for (uint32 Idx = 0; Idx < Regions.size(); Idx++)
		{
			DragRegions.Add(FChromiumWebBrowserDragRegion(
				FIntRect(Regions[Idx].bounds.x, Regions[Idx].bounds.y, Regions[Idx].bounds.x + Regions[Idx].bounds.width, Regions[Idx].bounds.y + Regions[Idx].bounds.height),
				Regions[Idx].draggable ? true : false));
		}
		BrowserWindow->UpdateDragRegions(DragRegions);
	}
}

bool FChromiumCEFBrowserHandler::CanSaveCookie(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	CefRefPtr<CefRequest> request,
	CefRefPtr<CefResponse> response,
	const CefCookie& cookie) 
{
	if (bAllowAllCookies)
	{
		return true;
	}

	// these two cookies shouldn't be saved by the client. While we are debugging why the backend is causing them to be set filter them out
	if (CefString(&cookie.name).ToString() == "store-token" || CefString(&cookie.name) == "EPIC_SESSION_DIESEL")
		return false;
	return true;
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_CEF
