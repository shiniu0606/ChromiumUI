// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEF/ChromiumCEFImeHandler.h"

#if WITH_CEF3 && !PLATFORM_LINUX

#include "ChromiumCEFTextInputMethodContext.h"


FChromiumCEFImeHandler::FChromiumCEFImeHandler(CefRefPtr<CefBrowser> Browser)
	: InternalCefBrowser(Browser)
	, TextInputMethodSystem(nullptr)
{

}

bool FChromiumCEFImeHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser, CefProcessId SourceProcess, CefRefPtr<CefProcessMessage> Message)
{
	bool Result = false;
	FString MessageName = WCHAR_TO_TCHAR(Message->GetName().ToWString().c_str());
	if (MessageName == TEXT("UE::IME::FocusChanged"))
	{
		Result = HandleFocusChangedMessage(Message->GetArgumentList());
	}
	return Result;
}

void FChromiumCEFImeHandler::SendProcessMessage(CefRefPtr<CefProcessMessage> Message)
{
	if (IsValid() && InternalCefBrowser->GetMainFrame())
	{
		InternalCefBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, Message);
	}
}

void FChromiumCEFImeHandler::BindInputMethodSystem(ITextInputMethodSystem* InTextInputMethodSystem)
{
	TextInputMethodSystem = InTextInputMethodSystem;
	if (TextInputMethodSystem && TextInputMethodContext.IsValid())
	{
		TextInputMethodChangeNotifier = TextInputMethodSystem->RegisterContext(TextInputMethodContext.ToSharedRef());
	}
}

void FChromiumCEFImeHandler::UnbindInputMethodSystem()
{
	if (TextInputMethodContext.IsValid())
	{
		DestroyContext();
	}
	TextInputMethodSystem = nullptr;
}

void FChromiumCEFImeHandler::InitContext()
{
	if (!TextInputMethodSystem)
	{
		return;
	}

	// Clean up any existing context
	DestroyContext();

	TextInputMethodContext = FChromiumCEFTextInputMethodContext::Create(SharedThis(this));
	if (TextInputMethodSystem)
	{
		TextInputMethodChangeNotifier = TextInputMethodSystem->RegisterContext(TextInputMethodContext.ToSharedRef());
	}
	if (TextInputMethodChangeNotifier.IsValid())
	{
		TextInputMethodChangeNotifier->NotifyLayoutChanged(ITextInputMethodChangeNotifier::ELayoutChangeType::Created);
	}

	TextInputMethodSystem->ActivateContext(TextInputMethodContext.ToSharedRef());
}

void FChromiumCEFImeHandler::DeactivateContext()
{
	if (!TextInputMethodSystem || !TextInputMethodContext.IsValid())
	{
		return;
	}

	TSharedRef<FChromiumCEFTextInputMethodContext> TextInputMethodContextRef = TextInputMethodContext.ToSharedRef();
	if (TextInputMethodSystem->IsActiveContext(TextInputMethodContextRef))
	{
		// Make sure that the composition is aborted to avoid any IME calls to EndComposition from trying to mutate our dying owner widget
		if (TextInputMethodContextRef->IsComposing())
		{
			TextInputMethodContextRef->AbortComposition();
			if (TextInputMethodChangeNotifier.IsValid())
			{
				TextInputMethodChangeNotifier->CancelComposition();
			}
		}
		TextInputMethodSystem->DeactivateContext(TextInputMethodContextRef);
	}
}

void FChromiumCEFImeHandler::DestroyContext()
{
	if (!TextInputMethodContext.IsValid())
	{
		return;
	}

	if (TextInputMethodSystem)
	{
		DeactivateContext();
		TextInputMethodSystem->UnregisterContext(TextInputMethodContext.ToSharedRef());
	}

	TextInputMethodChangeNotifier.Reset();
	TextInputMethodContext.Reset();
}

bool FChromiumCEFImeHandler::HandleFocusChangedMessage(CefRefPtr<CefListValue> MessageArguments)
{
	if (MessageArguments->GetSize() == 1 &&
		MessageArguments->GetType(0) == VTYPE_STRING)
	{
		if (TextInputMethodContext.IsValid())
		{
			DestroyContext();
		}
		return true;
	}
	else if (MessageArguments->GetSize() == 8 &&
		MessageArguments->GetType(0) == VTYPE_STRING &&
		MessageArguments->GetType(1) == VTYPE_STRING &&
		MessageArguments->GetType(2) == VTYPE_BOOL &&
		MessageArguments->GetType(3) == VTYPE_STRING &&
		MessageArguments->GetType(4) == VTYPE_INT &&
		MessageArguments->GetType(5) == VTYPE_INT &&
		MessageArguments->GetType(6) == VTYPE_INT &&
		MessageArguments->GetType(7) == VTYPE_INT)
	{
		FString Type = WCHAR_TO_TCHAR(MessageArguments->GetString(0).ToWString().c_str());
		FString Name = WCHAR_TO_TCHAR(MessageArguments->GetString(1).ToWString().c_str());
		bool bIsEditable = MessageArguments->GetBool(2);

		if (Type == TEXT("DOM_NODE_TYPE_ELEMENT") && 
			(Name == TEXT("INPUT") || Name == TEXT("TEXTAREA")) &&
			bIsEditable)
		{
			// @todo: Make use of the bounds of the text box here to act as a fallback for the initial composition window pos.
			InitContext();
		}
		return true;
	}

	return false;
}

void FChromiumCEFImeHandler::UnbindCefBrowser()
{
	if (TextInputMethodContext.IsValid())
	{
		DestroyContext();
	}
	InternalCefBrowser = nullptr;
}


void FChromiumCEFImeHandler::CacheBrowserSlateInfo(const TSharedRef<SWidget>& Widget)
{
	InternalBrowserSlateWidget = Widget;
}

void FChromiumCEFImeHandler::SetFocus(bool bHasFocus)
{
	if (!TextInputMethodSystem || !TextInputMethodContext.IsValid())
	{
		return;
	}

	if (bHasFocus)
	{
		TextInputMethodSystem->ActivateContext(TextInputMethodContext.ToSharedRef());
	}
	else
	{
		DeactivateContext();
	}
}

void FChromiumCEFImeHandler::UpdateCachedGeometry(const FGeometry& AllottedGeometry)
{
	if (TextInputMethodContext.IsValid() &&
		TextInputMethodContext->UpdateCachedGeometry(AllottedGeometry))
	{
		if (TextInputMethodChangeNotifier.IsValid())
		{
			TextInputMethodChangeNotifier->NotifyLayoutChanged(ITextInputMethodChangeNotifier::ELayoutChangeType::Changed);
		}
	}
}

void FChromiumCEFImeHandler::CEFCompositionRangeChanged(const CefRange& SelectionRange, const CefRenderHandler::RectList& CharacterBounds)
{
	if (TextInputMethodContext.IsValid() &&
		TextInputMethodContext->CEFCompositionRangeChanged(SelectionRange, CharacterBounds))
	{
		if (TextInputMethodChangeNotifier.IsValid())
		{
			TextInputMethodChangeNotifier->NotifyLayoutChanged(ITextInputMethodChangeNotifier::ELayoutChangeType::Changed);
		}
	}
}

#endif
