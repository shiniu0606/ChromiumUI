// Copyright Epic Games, Inc. All Rights Reserved.


#include "ChromiumNativeWebBrowserProxy.h"
#include "ChromiumNativeJSScripting.h"
#include "Misc/EmbeddedCommunication.h"


FChromiumNativeWebBrowserProxy::FChromiumNativeWebBrowserProxy(bool bInJSBindingToLoweringEnabled)
	: bJSBindingToLoweringEnabled(bInJSBindingToLoweringEnabled)
{

}

void FChromiumNativeWebBrowserProxy::Initialize()
{
	Scripting = MakeShareable(new FChromiumNativeJSScripting(bJSBindingToLoweringEnabled, SharedThis(this)));
	FEmbeddedDelegates::GetNativeToEmbeddedParamsDelegateForSubsystem(TEXT("browserProxy")).AddRaw(this, &FChromiumNativeWebBrowserProxy::HandleEmbeddedCommunication);
}

FChromiumNativeWebBrowserProxy::~FChromiumNativeWebBrowserProxy()
{
	FEmbeddedDelegates::GetNativeToEmbeddedParamsDelegateForSubsystem(TEXT("browserProxy")).RemoveAll(this);
}

bool FChromiumNativeWebBrowserProxy::OnJsMessageReceived(const FString& Message)
{
	return Scripting->OnJsMessageReceived(Message);
}

void FChromiumNativeWebBrowserProxy::HandleEmbeddedCommunication(const FEmbeddedCallParamsHelper& Params)
{
	FString Error;
	if (Params.Command == "handlejs")
	{
		FString Message = Params.Parameters.FindRef(TEXT("script"));
		if (!Message.IsEmpty())
		{
			if (!OnJsMessageReceived(Message))
			{
				Error = TEXT("Command failed");
			}
		}
	}
	else if (Params.Command == "pageload")
	{
		Scripting->PageLoaded();
	}

	Params.OnCompleteDelegate(FEmbeddedCommunicationMap(), Error);
}

void FChromiumNativeWebBrowserProxy::LoadURL(FString NewURL)
{
}

void FChromiumNativeWebBrowserProxy::LoadString(FString Contents, FString DummyURL)
{
}

void FChromiumNativeWebBrowserProxy::SetViewportSize(FIntPoint WindowSize, FIntPoint WindowPos)
{
}

FIntPoint FChromiumNativeWebBrowserProxy::GetViewportSize() const
{
	return FIntPoint(ForceInitToZero);
}

FSlateShaderResource* FChromiumNativeWebBrowserProxy::GetTexture(bool bIsPopup /*= false*/)
{
	return nullptr;
}

bool FChromiumNativeWebBrowserProxy::IsValid() const
{
	return false;
}

bool FChromiumNativeWebBrowserProxy::IsInitialized() const
{
	return true;
}

bool FChromiumNativeWebBrowserProxy::IsClosing() const
{
	return false;
}

EChromiumWebBrowserDocumentState FChromiumNativeWebBrowserProxy::GetDocumentLoadingState() const
{
	return EChromiumWebBrowserDocumentState::Loading;
}

FString FChromiumNativeWebBrowserProxy::GetTitle() const
{
	return TEXT("");
}

FString FChromiumNativeWebBrowserProxy::GetUrl() const
{
	return TEXT("");
}

void FChromiumNativeWebBrowserProxy::GetSource(TFunction<void(const FString&)> Callback) const
{
	Callback(FString());
}

void FChromiumNativeWebBrowserProxy::SetSupportsMouseWheel(bool bValue)
{

}

bool FChromiumNativeWebBrowserProxy::GetSupportsMouseWheel() const
{
	return false;
}

bool FChromiumNativeWebBrowserProxy::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	return false;
}

bool FChromiumNativeWebBrowserProxy::OnKeyUp(const FKeyEvent& InKeyEvent)
{
	return false;
}

bool FChromiumNativeWebBrowserProxy::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	return false;
}

FReply FChromiumNativeWebBrowserProxy::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumNativeWebBrowserProxy::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumNativeWebBrowserProxy::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

FReply FChromiumNativeWebBrowserProxy::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}

void FChromiumNativeWebBrowserProxy::OnMouseLeave(const FPointerEvent& MouseEvent)
{
}

FReply FChromiumNativeWebBrowserProxy::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup)
{
	return FReply::Unhandled();
}


void FChromiumNativeWebBrowserProxy::OnFocus(bool SetFocus, bool bIsPopup)
{
}

void FChromiumNativeWebBrowserProxy::OnCaptureLost()
{
}

bool FChromiumNativeWebBrowserProxy::CanGoBack() const
{
	return false;
}

void FChromiumNativeWebBrowserProxy::GoBack()
{
}

bool FChromiumNativeWebBrowserProxy::CanGoForward() const
{
	return false;
}

void FChromiumNativeWebBrowserProxy::GoForward()
{
}

bool FChromiumNativeWebBrowserProxy::IsLoading() const
{
	return false;
}

void FChromiumNativeWebBrowserProxy::Reload()
{
}

void FChromiumNativeWebBrowserProxy::StopLoad()
{
}

void FChromiumNativeWebBrowserProxy::ExecuteJavascript(const FString& Script)
{
	FEmbeddedCallParamsHelper CallHelper;
	CallHelper.Command = TEXT("execjs");
	CallHelper.Parameters = { { TEXT("script"), Script } };

	FEmbeddedDelegates::GetEmbeddedToNativeParamsDelegateForSubsystem(TEXT("webview")).Broadcast(CallHelper);
}

void FChromiumNativeWebBrowserProxy::CloseBrowser(bool bForce)
{
}

void FChromiumNativeWebBrowserProxy::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent /*= true*/)
{
	Scripting->BindUObject(Name, Object, bIsPermanent);
}

void FChromiumNativeWebBrowserProxy::UnbindUObject(const FString& Name, UObject* Object /*= nullptr*/, bool bIsPermanent /*= true*/)
{
	Scripting->UnbindUObject(Name, Object, bIsPermanent);
}

int FChromiumNativeWebBrowserProxy::GetLoadError()
{
	return 0;
}

void FChromiumNativeWebBrowserProxy::SetIsDisabled(bool bValue)
{
}

TSharedPtr<SWindow> FChromiumNativeWebBrowserProxy::GetParentWindow() const
{
	return nullptr;
}

void FChromiumNativeWebBrowserProxy::SetParentWindow(TSharedPtr<SWindow> Window)
{
}
