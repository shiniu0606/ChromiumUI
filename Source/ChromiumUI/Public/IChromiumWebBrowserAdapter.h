// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IChromiumWebBrowserWindow.h"

class IChromiumWebBrowserAdapter
{
public:

	virtual FString GetName() const = 0;

	virtual bool IsPermanent() const = 0;

	virtual void ConnectTo(const TSharedRef<IChromiumWebBrowserWindow>& BrowserWindow) = 0;

	virtual void DisconnectFrom(const TSharedRef<IChromiumWebBrowserWindow>& BrowserWindow) = 0;

};

class CHROMIUMUI_API FChromiumWebBrowserAdapterFactory
{ 
public: 

	static TSharedRef<IChromiumWebBrowserAdapter> Create(const FString& Name, UObject* JSBridge, bool IsPermanent);

	static TSharedRef<IChromiumWebBrowserAdapter> Create(const FString& Name, UObject* JSBridge, bool IsPermanent, const FString& ConnectScriptText, const FString& DisconnectScriptText);
}; 
