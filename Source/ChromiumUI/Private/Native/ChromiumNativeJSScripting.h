// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "ChromiumWebJSFunction.h"
#include "ChromiumWebJSScripting.h"

typedef TSharedRef<class FChromiumNativeJSScripting> FChromiumNativeJSScriptingRef;
typedef TSharedPtr<class FChromiumNativeJSScripting> FChromiumNativeJSScriptingPtr;

class FChromiumNativeWebBrowserProxy;

/**
 * Implements handling of bridging UObjects client side with JavaScript renderer side.
 */
class FChromiumNativeJSScripting
	: public FChromiumWebJSScripting
	, public TSharedFromThis<FChromiumNativeJSScripting>
{
public:
	//static const FString JSMessageTag;

	FChromiumNativeJSScripting(bool bJSBindingToLoweringEnabled, TSharedRef<FChromiumNativeWebBrowserProxy> Window);

	virtual void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) override;
	virtual void UnbindUObject(const FString& Name, UObject* Object = nullptr, bool bIsPermanent = true) override;

	bool OnJsMessageReceived(const FString& Message);

	FString ConvertStruct(UStruct* TypeInfo, const void* StructPtr);
	FString ConvertObject(UObject* Object);

	virtual void InvokeJSFunction(FGuid FunctionId, int32 ArgCount, FChromiumWebJSParam Arguments[], bool bIsError=false) override;
	virtual void InvokeJSErrorResult(FGuid FunctionId, const FString& Error) override;
	void PageLoaded();

private:
	FString GetInitializeScript();
	void InvokeJSFunctionRaw(FGuid FunctionId, const FString& JSValue, bool bIsError=false);
	bool IsValid()
	{
		return WindowPtr.Pin().IsValid();
	}

	/** Message handling helpers */
	bool HandleExecuteUObjectMethodMessage(const TArray<FString>& Params);
	void ExecuteJavascript(const FString& Javascript);

	TWeakPtr<FChromiumNativeWebBrowserProxy> WindowPtr;
	bool bLoaded;
};
