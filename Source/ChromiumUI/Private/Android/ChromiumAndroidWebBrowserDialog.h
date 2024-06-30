// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if USE_ANDROID_JNI

#include "IChromiumWebBrowserDialog.h"

#include <jni.h>

class SChromiumAndroidWebBrowserWidget;

class FChromiumAndroidWebBrowserDialog
	: public IChromiumWebBrowserDialog
{
public:
	virtual ~FChromiumAndroidWebBrowserDialog()
	{}

	// IWebBrowserDialog interface:

	virtual EChromiumWebBrowserDialogType GetType() override
	{
		return Type;
	}

	virtual const FText& GetMessageText() override
	{
		return MessageText;
	}

	virtual const FText& GetDefaultPrompt() override
	{
		return DefaultPrompt;
	}

	virtual bool IsReload() override
	{
		check(Type == EChromiumWebBrowserDialogType::Unload);
		return false; // The android webkit browser does not provide this infomation
	}

	virtual void Continue(bool Success = true, const FText& UserResponse = FText::GetEmpty()) override;

private:

	EChromiumWebBrowserDialogType Type;
	FText MessageText;
	FText DefaultPrompt;

	jobject Callback; // Either a reference to a JsResult or a JsPromptResult object depending on Type

	// Create a dialog from OnJSPrompt arguments
	FChromiumAndroidWebBrowserDialog(jstring InMessageText, jstring InDefaultPrompt, jobject InCallback);

	// Create a dialog from OnJSAlert|Confirm|BeforeUnload arguments
	FChromiumAndroidWebBrowserDialog(EChromiumWebBrowserDialogType InDialogType, jstring InMessageText, jobject InCallback);

	friend class FAndroidWebBrowserWindow;
	friend class SChromiumAndroidWebBrowserWidget;
};

typedef FChromiumAndroidWebBrowserDialog FChromiumWebBrowserDialog;

#endif // USE_ANDROID_JNI