// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rendering/SlateRenderer.h"
#include "IChromiumWebBrowserResourceLoader.h"
class FChromiumCEFWebBrowserWindow;
class IChromiumWebBrowserCookieManager;
class IChromiumWebBrowserWindow;
class IChromiumWebBrowserSchemeHandlerFactory;
class UMaterialInterface;
struct FChromiumWebBrowserWindowInfo;

class IChromiumWebBrowserWindowFactory
{
public:

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo) = 0;

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		void* OSWindowHandle,
		FString InitialURL,
		bool bUseTransparency,
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(),
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255)) = 0;
};

struct CHROMIUMUI_API FChromiumBrowserContextSettings
{
	FChromiumBrowserContextSettings(const FString& InId)
		: Id(InId)
		, AcceptLanguageList()
		, CookieStorageLocation()
		, bPersistSessionCookies(false)
		, bIgnoreCertificateErrors(false)
		, bEnableNetSecurityExpiration(true)
	{ }

	FString Id;
	FString AcceptLanguageList;
	FString CookieStorageLocation;
	bool bPersistSessionCookies;
	bool bIgnoreCertificateErrors;
	bool bEnableNetSecurityExpiration;
	FChromiumOnBeforeContextResourceLoadDelegate OnBeforeContextResourceLoad;
};


struct CHROMIUMUI_API FChromiumCreateBrowserWindowSettings
{

	FChromiumCreateBrowserWindowSettings()
		: OSWindowHandle(nullptr)
		, InitialURL()
		, bUseTransparency(false)
		, bThumbMouseButtonNavigation(false)
		, ContentsToLoad()
		, bShowErrorMessage(true)
		, BackgroundColor(FColor(0, 0, 0, 0))
		, BrowserFrameRate(24)
		, Context()
		, AltRetryDomains()
	{ }

	void* OSWindowHandle;
	FString InitialURL;
	bool bUseTransparency;
	bool bThumbMouseButtonNavigation;
	TOptional<FString> ContentsToLoad;
	bool bShowErrorMessage;
	FColor BackgroundColor;
	int BrowserFrameRate;
	TOptional<FChromiumBrowserContextSettings> Context;
	TArray<FString> AltRetryDomains;
};

/**
 * A singleton class that takes care of general web browser tasks
 */
class CHROMIUMUI_API IChromiumWebBrowserSingleton
{
public:
	/**
	 * Virtual Destructor
	 */
	virtual ~IChromiumWebBrowserSingleton() {};

	/** @return A factory object that can be used to construct additional WebBrowserWindows on demand. */
	virtual TSharedRef<IChromiumWebBrowserWindowFactory> GetWebBrowserWindowFactory() const = 0;


	/**
	 * Create a new web browser window
	 *
	 * @param BrowserWindowParent The parent browser window
	 * @param BrowserWindowInfo Info for setting up the new browser window
	 * @return New Web Browser Window Interface (may be null if not supported)
	 */
	virtual TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(
		TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo
		) = 0;

	/**
	 * Create a new web browser window
	 *
	 * @param OSWindowHandle Handle of OS Window that the browser will be displayed in (can be null)
	 * @param InitialURL URL that the browser should initially navigate to
	 * @param bUseTransparency Whether to allow transparent rendering of pages
	 * @param ContentsToLoad Optional string to load as a web page
	 * @param ShowErrorMessage Whether to show an error message in case of loading errors.
	 * @param BackgroundColor Opaque background color used before a document is loaded and when no document color is specified.
	 * @param BrowserFrameRate The framerate of the browser in FPS
	 * @return New Web Browser Window Interface (may be null if not supported)
	 */
	UE_DEPRECATED(4.11, "Please use the new overload that takes a settings struct.")
	virtual TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(
		void* OSWindowHandle,
		FString InitialURL,
		bool bUseTransparency,
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(),
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255),
		int BrowserFrameRate = 24,
		const TArray<FString>& AltRetryDomains = TArray<FString>()) = 0;

	virtual TSharedPtr<IChromiumWebBrowserWindow> CreateBrowserWindow(const FChromiumCreateBrowserWindowSettings& Settings) = 0;

#if	BUILD_EMBEDDED_APP
	virtual TSharedPtr<IWebBrowserWindow> CreateNativeBrowserProxy() = 0;
#endif

	/**
	 * Delete all browser cookies.
	 *
	 * Removes all matching cookies. Leave both URL and CookieName blank to delete the entire cookie database.
	 * The actual deletion will be scheduled on the browser IO thread, so the operation may not have completed when the function returns.
	 *
	 * @param URL The base URL to match when searching for cookies to remove. Use blank to match all URLs.
	 * @param CookieName The name of the cookie to delete. If left unspecified, all cookies will be removed.
	 * @param Completed A callback function that will be invoked asynchronously on the game thread when the deletion is complete passing in the number of deleted objects.
	 */
	UE_DEPRECATED(4.11, "Please use the CookieManager instead via GetCookieManager().")
	virtual void DeleteBrowserCookies(FString URL = TEXT(""), FString CookieName = TEXT(""), TFunction<void(int)> Completed = nullptr) = 0;

	virtual TSharedPtr<class IChromiumWebBrowserCookieManager> GetCookieManager() const = 0;

	virtual TSharedPtr<class IChromiumWebBrowserCookieManager> GetCookieManager(TOptional<FString> ContextId) const = 0;

	virtual bool RegisterContext(const FChromiumBrowserContextSettings& Settings) = 0;

	virtual bool UnregisterContext(const FString& ContextId) = 0;

	// @return the application cache dir where the cookies are stored
	virtual FString ApplicationCacheDir() const = 0;
	/**
	 * Registers a custom scheme handler factory, for a given scheme and domain. The domain is ignored if the scheme is not a browser built in scheme
	 * and all requests will go through this factory.
	 * @param Scheme                            The scheme name to handle.
	 * @param Domain                            The domain name to handle.
	 * @param WebBrowserSchemeHandlerFactory    The factory implementation for creating request handlers for this scheme.
	 */
	virtual bool RegisterSchemeHandlerFactory(FString Scheme, FString Domain, IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory) = 0;

	/**
	 * Unregister a custom scheme handler factory. The factory may still be used by existing open browser windows, but will no longer be provided for new ones.
	 * @param WebBrowserSchemeHandlerFactory    The factory implementation to remove.
	 */
	virtual bool UnregisterSchemeHandlerFactory(IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory) = 0;

	/**
	 * Enable or disable CTRL/CMD-SHIFT-I shortcut to show the Chromium Dev tools window.
	 * The value defaults to true on debug builds, otherwise false.
	 *
	 * The relevant handlers for spawning new browser windows have to be set up correctly in addition to this flag being true before anything is shown.
	 *
	 * @param Value a boolean value to enable or disable the keyboard shortcut.
	 */
	virtual void SetDevToolsShortcutEnabled(bool Value) = 0;


	/**
	 * Returns whether the CTRL/CMD-SHIFT-I shortcut to show the Chromium Dev tools window is enabled.
	 *
	 * The relevant handlers for spawning new browser windows have to be set up correctly in addition to this flag being true before anything is shown.
	 *
	 * @return a boolean value indicating whether the keyboard shortcut is enabled or not.
	 */
	virtual bool IsDevToolsShortcutEnabled() = 0;


	/**
	 * Enable or disable to-lowering of JavaScript object member bindings.
	 *
	 * Due to how JavaScript to UObject bridges require the use of FNames for building up the JS API objects, it is possible for case-sensitivity issues
	 * to develop if an FName has been previously created with differing case to your function or property names. To-lowering the member names allows
	 * a guaranteed casing for the web page's JS to reference.
	 *
	 * Default behavior is enabled, so that all JS side objects have only lowercase members.
	 *
	 * @param bEnabled a boolean value to enable or disable the to-lowering.
	 */
	virtual void SetJSBindingToLoweringEnabled(bool bEnabled) = 0;


	/**
	 * Delete old/unused web cache paths. Some Web implementations (i.e CEF) use version specific cache folders, this
	 * call lets you schedule a deletion of any now unused folders. Calling this may resulting in async disk I/O.
	 *
	 * @param CachePathRoot the base path used for saving the webcache folder
	 * @param CachePrefix the filename prefix we use for the cache folder (i.e "webcache")
	 */
	virtual void ClearOldCacheFolders(const FString &CachePathRoot, const FString &CachePrefix) = 0;


	/** Set a reference to UWebBrowser's default material*/
	virtual void SetDefaultMaterial(UMaterialInterface* InDefaultMaterial) = 0;
	/** Set a reference to UWebBrowser's translucent material*/
	virtual void SetDefaultTranslucentMaterial(UMaterialInterface* InDefaultMaterial) = 0;

	/** Get a reference to UWebBrowser's default material*/
	virtual UMaterialInterface* GetDefaultMaterial() = 0;
	/** Get a reference to UWebBrowser's transparent material*/
	virtual UMaterialInterface* GetDefaultTranslucentMaterial() = 0;
};
