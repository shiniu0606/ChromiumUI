// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID  || PLATFORM_IOS

#include "ChromiumWebJSFunction.h"
#include "ChromiumWebJSScripting.h"

typedef TSharedRef<class FChromiumMobileJSScripting> FChromiumMobileJSScriptingRef;
typedef TSharedPtr<class FChromiumMobileJSScripting> FChromiumMobileJSScriptingPtr;

/**
 * Implements handling of bridging UObjects client side with JavaScript renderer side.
 */
class FChromiumMobileJSScripting
	: public FChromiumWebJSScripting
	, public TSharedFromThis<FChromiumMobileJSScripting>
{
public:
	static const FString JSMessageTag;
	static const FString JSMessageHandler;

	FChromiumMobileJSScripting(bool bJSBindingToLoweringEnabled);

	virtual void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) override;
	virtual void UnbindUObject(const FString& Name, UObject* Object = nullptr, bool bIsPermanent = true) override;
	void BindUObject(TSharedRef<class IChromiumWebBrowserWindow> InWindow, const FString& Name, UObject* Object, bool bIsPermanent = true);
	void UnbindUObject(TSharedRef<class IChromiumWebBrowserWindow> InWindow, const FString& Name, UObject* Object = nullptr, bool bIsPermanent = true);

	/**
	 * Called when a message was received from the browser process.
	 *
	 * @param Command The command sent from the browser.
	 * @param Params Command-specific data.
	 * @return true if the message was handled, else false.
	 */
	bool OnJsMessageReceived(const FString& Command, const TArray<FString>& Params, const FString& Origin);

	FString ConvertStruct(UStruct* TypeInfo, const void* StructPtr);
	FString ConvertObject(UObject* Object);

	virtual void InvokeJSFunction(FGuid FunctionId, int32 ArgCount, FChromiumWebJSParam Arguments[], bool bIsError=false) override;
	virtual void InvokeJSErrorResult(FGuid FunctionId, const FString& Error) override;
	void PageLoaded(TSharedRef<class IChromiumWebBrowserWindow> InWindow); // Called on page load

	void SetWindow(TSharedRef<class IChromiumWebBrowserWindow> InWindow);

private:
	void InitializeScript(TSharedRef<class IChromiumWebBrowserWindow> InWindow);
	void InvokeJSFunctionRaw(FGuid FunctionId, const FString& JSValue, bool bIsError=false);
	bool IsValid()
	{
		return WindowPtr.Pin().IsValid();
	}
	void AddPermanentBind(const FString& Name, UObject* Object);
	void RemovePermanentBind(const FString& Name, UObject* Object);

	/** Message handling helpers */

	bool HandleExecuteUObjectMethodMessage(const TArray<FString>& Params);

	/** Pointer to the Mobile Browser for this window. */
	TWeakPtr<class IChromiumWebBrowserWindow> WindowPtr;
};

#endif // PLATFORM_ANDROID  || PLATFORM_IOS