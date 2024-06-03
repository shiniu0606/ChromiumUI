// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumAndroidWebBrowserWindow.h"

#if USE_ANDROID_JNI

#include "ChromiumAndroidWebBrowserDialog.h"
#include "ChromiumAndroidWebBrowserWidget.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidWindow.h"
#include "Android/AndroidJava.h"

#include <jni.h>


namespace {

	static const FString JSGetSourceCommand = TEXT("GetSource");
	static const FString JSMessageGetSourceScript =
		TEXT("document.location = '") + FChromiumMobileJSScripting::JSMessageTag + JSGetSourceCommand +
		TEXT("/' + encodeURIComponent(document.documentElement.innerHTML);");

}

FChromiumAndroidWebBrowserWindow::FChromiumAndroidWebBrowserWindow(FString InUrl, TOptional<FString> InContentsToLoad, bool InShowErrorMessage, bool InThumbMouseButtonNavigation, bool InUseTransparency, bool bInJSBindingToLoweringEnabled)
	: CurrentUrl(MoveTemp(InUrl))
	, ContentsToLoad(MoveTemp(InContentsToLoad))
	, bUseTransparency(InUseTransparency)
	, DocumentState(EChromiumWebBrowserDocumentState::NoDocument)
	, ErrorCode(0)
	, Scripting(new FChromiumMobileJSScripting(bInJSBindingToLoweringEnabled))
	, AndroidWindowSize(FIntPoint(500, 500))
	, bIsDisabled(false)
	, bIsVisible(true)
	, bTickedLastFrame(true)
{
}

FChromiumAndroidWebBrowserWindow::~FChromiumAndroidWebBrowserWindow()
{
	CloseBrowser(true);
}

void FChromiumAndroidWebBrowserWindow::LoadURL(FString NewURL)
{
	BrowserWidget->LoadURL(NewURL);
}

void FChromiumAndroidWebBrowserWindow::LoadString(FString Contents, FString DummyURL)
{
	BrowserWidget->LoadString(Contents, DummyURL);
}

TSharedRef<SWidget> FChromiumAndroidWebBrowserWindow::CreateWidget()
{
	TSharedRef<SChromiumAndroidWebBrowserWidget> BrowserWidgetRef =
		SNew(SChromiumAndroidWebBrowserWidget)
		.UseTransparency(bUseTransparency)
		.InitialURL(CurrentUrl)
		.WebBrowserWindow(SharedThis(this));

	BrowserWidget = BrowserWidgetRef;

	Scripting->SetWindow(SharedThis(this));
		
	return BrowserWidgetRef;
}

void FChromiumAndroidWebBrowserWindow::SetViewportSize(FIntPoint WindowSize, FIntPoint WindowPos)
{
	AndroidWindowSize = WindowSize;
}

FIntPoint FChromiumAndroidWebBrowserWindow::GetViewportSize() const
{
	return AndroidWindowSize;
}

FSlateShaderResource* FChromiumAndroidWebBrowserWindow::GetTexture(bool bIsPopup /*= false*/)
{
	return nullptr;
}

bool FChromiumAndroidWebBrowserWindow::IsValid() const
{
	return false;
}

bool FChromiumAndroidWebBrowserWindow::IsInitialized() const
{
	return true;
}

bool FChromiumAndroidWebBrowserWindow::IsClosing() const
{
	return false;
}

EChromiumWebBrowserDocumentState FChromiumAndroidWebBrowserWindow::GetDocumentLoadingState() const
{
	return DocumentState;
}

FString FChromiumAndroidWebBrowserWindow::GetTitle() const
{
	return Title;
}

FString FChromiumAndroidWebBrowserWindow::GetUrl() const
{
	return CurrentUrl;
}

bool FChromiumAndroidWebBrowserWindow::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	return false;
}

bool FChromiumAndroidWebBrowserWindow::OnKeyUp(const FKeyEvent& InKeyEvent)
{
	return false;
}

bool FChromiumAndroidWebBrowserWindow::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	return false;
}

FReply FChromiumAndroidWebBrowserWindow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumAndroidWebBrowserWindow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumAndroidWebBrowserWindow::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumAndroidWebBrowserWindow::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

void FChromiumAndroidWebBrowserWindow::OnMouseLeave(const FPointerEvent& MouseEvent)
{
}

void FChromiumAndroidWebBrowserWindow::SetSupportsMouseWheel(bool bValue)
{
}

bool FChromiumAndroidWebBrowserWindow::GetSupportsMouseWheel() const
{
	return false;
}

FReply FChromiumAndroidWebBrowserWindow::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

void FChromiumAndroidWebBrowserWindow::OnFocus(bool SetFocus, bool bIsPopup)
{
}

void FChromiumAndroidWebBrowserWindow::OnCaptureLost()
{
}

bool FChromiumAndroidWebBrowserWindow::CanGoBack() const
{
	return BrowserWidget->CanGoBack();
}

void FChromiumAndroidWebBrowserWindow::GoBack()
{
	BrowserWidget->GoBack();
}

bool FChromiumAndroidWebBrowserWindow::CanGoForward() const
{
	return BrowserWidget->CanGoForward();
}

void FChromiumAndroidWebBrowserWindow::GoForward()
{
	BrowserWidget->GoForward();
}

bool FChromiumAndroidWebBrowserWindow::IsLoading() const
{
	return DocumentState != EChromiumWebBrowserDocumentState::Loading;
}

void FChromiumAndroidWebBrowserWindow::Reload()
{
	BrowserWidget->Reload();
}

void FChromiumAndroidWebBrowserWindow::StopLoad()
{
	BrowserWidget->StopLoad();
}

void FChromiumAndroidWebBrowserWindow::GetSource(TFunction<void (const FString&)> Callback) const
{
	//@todo: decide what to do about multiple pending requests
	GetPageSourceCallback.Emplace(Callback);

	// Ugly hack: Work around the fact that ExecuteJavascript is non-const.
	const_cast<FChromiumAndroidWebBrowserWindow*>(this)->ExecuteJavascript(JSMessageGetSourceScript);
}

int FChromiumAndroidWebBrowserWindow::GetLoadError()
{
	return ErrorCode;
}

void FChromiumAndroidWebBrowserWindow::NotifyDocumentError(const FString& InCurrentUrl, int InErrorCode)
{
	if(!CurrentUrl.Equals(InCurrentUrl, ESearchCase::CaseSensitive))
	{
		CurrentUrl = InCurrentUrl;
		UrlChangedEvent.Broadcast(CurrentUrl);
	}

	ErrorCode = InErrorCode;
	DocumentState = EChromiumWebBrowserDocumentState::Error;
	DocumentStateChangedEvent.Broadcast(DocumentState);
}

void FChromiumAndroidWebBrowserWindow::NotifyDocumentLoadingStateChange(const FString& InCurrentUrl, bool IsLoading)
{
	// Ignore a load completed notification if there was an error.
	// For load started, reset any errors from previous page load.
	if (IsLoading || DocumentState != EChromiumWebBrowserDocumentState::Error)
	{
		if(!CurrentUrl.Equals(InCurrentUrl, ESearchCase::CaseSensitive))
		{
			CurrentUrl = InCurrentUrl;
			UrlChangedEvent.Broadcast(CurrentUrl);
		}

		if(!IsLoading && !InCurrentUrl.StartsWith("javascript:"))
		{
			Scripting->PageLoaded(SharedThis(this));
		}
		ErrorCode = 0;
		DocumentState = IsLoading
			? EChromiumWebBrowserDocumentState::Loading
			: EChromiumWebBrowserDocumentState::Completed;
		DocumentStateChangedEvent.Broadcast(DocumentState);
	}

}

void FChromiumAndroidWebBrowserWindow::SetIsDisabled(bool bValue)
{
	bIsDisabled = bValue;
}

TSharedPtr<SWindow> FChromiumAndroidWebBrowserWindow::GetParentWindow() const
{
	return ParentWindow;
}

void FChromiumAndroidWebBrowserWindow::SetParentWindow(TSharedPtr<SWindow> Window)
{
	ParentWindow = Window;
}

void FChromiumAndroidWebBrowserWindow::ExecuteJavascript(const FString& Script)
{
	BrowserWidget->ExecuteJavascript(Script);
}

void FChromiumAndroidWebBrowserWindow::CloseBrowser(bool bForce)
{
	BrowserWidget->Close();
}

bool FChromiumAndroidWebBrowserWindow::OnJsMessageReceived(const FString& Command, const TArray<FString>& Params, const FString& Origin)
{
	if( Command.Equals(JSGetSourceCommand, ESearchCase::CaseSensitive) && GetPageSourceCallback.IsSet() && Params.Num() == 1)
	{
		GetPageSourceCallback.GetValue()(Params[0]);
		GetPageSourceCallback.Reset();
		return true;
	}
	return Scripting->OnJsMessageReceived(Command, Params, Origin);
}

void FChromiumAndroidWebBrowserWindow::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent /*= true*/)
{
	Scripting->BindUObject(SharedThis(this), Name, Object, bIsPermanent);
}

void FChromiumAndroidWebBrowserWindow::UnbindUObject(const FString& Name, UObject* Object /*= nullptr*/, bool bIsPermanent /*= true*/)
{
	Scripting->UnbindUObject(SharedThis(this), Name, Object, bIsPermanent);
}

void FChromiumAndroidWebBrowserWindow::CheckTickActivity()
{
	if (bIsVisible != bTickedLastFrame)
	{
		bIsVisible = bTickedLastFrame;
		BrowserWidget->SetWebBrowserVisibility(bIsVisible);
	}

	bTickedLastFrame = false;
}

void FChromiumAndroidWebBrowserWindow::SetTickLastFrame()
{
	bTickedLastFrame = !bIsDisabled;
}

bool FChromiumAndroidWebBrowserWindow::IsVisible()
{
	return bIsVisible;
}

#endif // USE_ANDROID_JNI