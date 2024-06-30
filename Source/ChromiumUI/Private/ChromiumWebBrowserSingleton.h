// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "IChromiumWebBrowserSingleton.h"

#if WITH_CEF3
#if PLATFORM_WINDOWS
	#include "Windows/WindowsHWrapper.h"
	#include "Windows/AllowWindowsPlatformTypes.h"
	#include "Windows/AllowWindowsPlatformAtomics.h"
#endif
#pragma push_macro("OVERRIDE")
#undef OVERRIDE // cef headers provide their own OVERRIDE macro
THIRD_PARTY_INCLUDES_START
#if PLATFORM_APPLE
PRAGMA_DISABLE_DEPRECATION_WARNINGS
#endif
#include "include/internal/cef_ptr.h"
#include "include/cef_request_context.h"
#if PLATFORM_APPLE
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
THIRD_PARTY_INCLUDES_END
#pragma pop_macro("OVERRIDE")
#if PLATFORM_WINDOWS
	#include "Windows/HideWindowsPlatformAtomics.h"
	#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "CEF/ChromiumCEFSchemeHandler.h"
#include "CEF/ChromiumCEFResourceContextHandler.h"
class CefListValue;
class FChromiumCEFBrowserApp;
class FChromiumCEFWebBrowserWindow;
#endif

class IChromiumWebBrowserCookieManager;
class IChromiumWebBrowserWindow;
struct FChromiumWebBrowserWindowInfo;
struct FChromiumWebBrowserInitSettings;
class UMaterialInterface;

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/**
 * Implementation of singleton class that takes care of general web browser tasks
 */
class FChromiumWebBrowserSingleton
	: public IChromiumWebBrowserSingleton
	, public FTickerObjectBase
{
public:

	/** Constructor. */
	FChromiumWebBrowserSingleton(const FChromiumWebBrowserInitSettings& WebBrowserInitSettings);

	/** Virtual destructor. */
	virtual ~FChromiumWebBrowserSingleton();

	/**
	* Gets the Current Locale Code in the format CEF expects
	*
	* @return Locale code as either "xx" or "xx-YY"
	*/
	static FString GetCurrentLocaleCode();

	virtual FString ApplicationCacheDir() const override;

public:

	// IWebBrowserSingleton Interface

	virtual TSharedRef<IChromiumWebBrowserWindowFactory> GetWebBrowserWindowFactory() const override;

	TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(
		TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo) override;

	TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(
		void* OSWindowHandle,
		FString InitialURL,
		bool bUseTransparency,
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(),
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255),
		int BrowserFrameRate = 24,
		const TArray<FString>& AltRetryDomains = TArray<FString>()) override;

	TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(const FChromiumCreateBrowserWindowSettings& Settings) override;

#if	BUILD_EMBEDDED_APP
	TSharedPtr<IChromiumWebBrowserWindow> CreateNativeBrowserProxy() override;
#endif

	virtual void DeleteBrowserCookies(FString URL = TEXT(""), FString CookieName = TEXT(""), TFunction<void(int)> Completed = nullptr) override;

	virtual TSharedPtr<IChromiumWebBrowserCookieManager> GetCookieManager() const override
	{
		return DefaultCookieManager;
	}

	virtual TSharedPtr<IChromiumWebBrowserCookieManager> GetCookieManager(TOptional<FString> ContextId) const override;

	virtual bool RegisterContext(const FChromiumBrowserContextSettings& Settings) override;

	virtual bool UnregisterContext(const FString& ContextId) override;

	virtual bool RegisterSchemeHandlerFactory(FString Scheme, FString Domain, IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory) override;

	virtual bool UnregisterSchemeHandlerFactory(IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory) override;

	virtual bool IsDevToolsShortcutEnabled() override
	{
		return bDevToolsShortcutEnabled;
	}

	virtual void SetDevToolsShortcutEnabled(bool Value) override
	{
		bDevToolsShortcutEnabled = Value;
	}

	virtual void SetJSBindingToLoweringEnabled(bool bEnabled) override
	{
		bJSBindingsToLoweringEnabled = bEnabled;
	}

	virtual void ClearOldCacheFolders(const FString& CachePathRoot, const FString& CachePrefix) override;

	/** Set a reference to UWebBrowser's default material*/
	virtual void SetDefaultMaterial(UMaterialInterface* InDefaultMaterial) override
	{
		DefaultMaterial = InDefaultMaterial;
	}

	/** Set a reference to UWebBrowser's translucent material*/
	virtual void SetDefaultTranslucentMaterial(UMaterialInterface* InDefaultMaterial) override
	{
		DefaultTranslucentMaterial = InDefaultMaterial;
	}

	/** Get a reference to UWebBrowser's default material*/
	virtual UMaterialInterface* GetDefaultMaterial() override
	{
		return DefaultMaterial;
	}

	/** Get a reference to UWebBrowser's translucent material*/
	virtual UMaterialInterface* GetDefaultTranslucentMaterial() override
	{
		return DefaultTranslucentMaterial;
	}

public:

	// FTSTickerObjectBase Interface

	virtual bool Tick(float DeltaTime) override;

private:

	TSharedPtr<IChromiumWebBrowserCookieManager> DefaultCookieManager;

#if WITH_CEF3
	/** When new render processes are created, send all permanent variable bindings to them. */
	void HandleRenderProcessCreated(CefRefPtr<CefListValue> ExtraInfo);
	/** Helper function to generate the CEF build unique name for the cache_path */
	FString GenerateWebCacheFolderName(const FString &InputPath);
	/** Pointer to the CEF App implementation */
	CefRefPtr<FChromiumCEFBrowserApp>			CEFBrowserApp;

	TMap<FString, CefRefPtr<CefRequestContext>> RequestContexts;
	TMap<FString, CefRefPtr<FChromiumCEFResourceContextHandler>> RequestResourceHandlers;
	FChromiumCefSchemeHandlerFactories SchemeHandlerFactories;
	bool bAllowCEF;
	bool bTaskFinished;
#endif

	/** List of currently existing browser windows */
#if WITH_CEF3
	TArray<TWeakPtr<FChromiumCEFWebBrowserWindow>>	WindowInterfaces;
#elif PLATFORM_IOS || PLATFORM_PS4 || (PLATFORM_ANDROID && USE_ANDROID_JNI)
	TArray<TWeakPtr<IChromiumWebBrowserWindow>>	WindowInterfaces;
#endif

	/** Critical section for thread safe modification of WindowInterfaces array. */
	FCriticalSection WindowInterfacesCS;

	TSharedRef<IChromiumWebBrowserWindowFactory> WebBrowserWindowFactory;

	bool bDevToolsShortcutEnabled;

	bool bJSBindingsToLoweringEnabled;

	/** Reference to UWebBrowser's default material*/
	UMaterialInterface* DefaultMaterial;

	/** Reference to UWebBrowser's translucent material*/
	UMaterialInterface* DefaultTranslucentMaterial;

};

PRAGMA_ENABLE_DEPRECATION_WARNINGS

#if WITH_CEF3

class CefCookieManager;

class FChromiumCefWebBrowserCookieManagerFactory
{
public:
	static TSharedRef<IChromiumWebBrowserCookieManager> Create(
		const CefRefPtr<CefCookieManager>& CookieManager);
};

#endif
