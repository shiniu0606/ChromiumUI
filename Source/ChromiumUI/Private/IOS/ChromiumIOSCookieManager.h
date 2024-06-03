// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_IOS
#include "IChromiumWebBrowserCookieManager.h"

/**
 * Implementation of interface for dealing with a Web Browser cookies for iOS.
 */
class FChromiumIOSCookieManager
	: public IChromiumWebBrowserCookieManager
	, public TSharedFromThis<FChromiumIOSCookieManager>
{
public:

	// IWebBrowserCookieManager interface

	virtual void SetCookie(const FString& URL, const FChromiumCookie& Cookie, TFunction<void(bool)> Completed = nullptr) override;
	virtual void DeleteCookies(const FString& URL = TEXT(""), const FString& CookieName = TEXT(""), TFunction<void(int)> Completed = nullptr) override;

	// FIOSCookieManager

	FChromiumIOSCookieManager();
	virtual ~FChromiumIOSCookieManager();
};
#endif
