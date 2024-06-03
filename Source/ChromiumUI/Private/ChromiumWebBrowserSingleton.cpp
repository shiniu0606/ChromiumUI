// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumWebBrowserSingleton.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Internationalization/Culture.h"
#include "Misc/App.h"
#include "ChromiumWebBrowserModule.h"
#include "Misc/EngineVersion.h"
#include "Framework/Application/SlateApplication.h"
#include "IChromiumWebBrowserCookieManager.h"
#include "ChromiumWebBrowserLog.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

#if WITH_CEF3
#include "Misc/ScopeLock.h"
#include "Async/Async.h"
#include "CEF/ChromiumCEFBrowserApp.h"
#include "CEF/ChromiumCEFBrowserHandler.h"
#include "CEF/ChromiumCEFWebBrowserWindow.h"
#include "CEF/ChromiumCEFSchemeHandler.h"
#include "CEF/ChromiumCEFResourceContextHandler.h"
#include "CEF/ChromiumCEFBrowserClosureTask.h"
#	if PLATFORM_WINDOWS
#		include "Windows/AllowWindowsPlatformTypes.h"
#	endif
#	pragma push_macro("OVERRIDE")
#		undef OVERRIDE // cef headers provide their own OVERRIDE macro
THIRD_PARTY_INCLUDES_START
#if PLATFORM_APPLE
PRAGMA_DISABLE_DEPRECATION_WARNINGS
#endif
#		include "include/cef_app.h"
#		include "include/cef_version.h"
#if PLATFORM_APPLE
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
THIRD_PARTY_INCLUDES_END
#	pragma pop_macro("OVERRIDE")
#	if PLATFORM_WINDOWS
#		include "Windows/HideWindowsPlatformTypes.h"
#	endif
#endif

#if BUILD_EMBEDDED_APP
#	include "Native/ChromiumNativeWebBrowserProxy.h"
#endif

#if PLATFORM_ANDROID && USE_ANDROID_JNI
#	include "Android/ChromiumAndroidWebBrowserWindow.h"
#	include <Android/ChromiumAndroidCookieManager.h>
#elif PLATFORM_IOS
#	include <IOS/ChromiumIOSPlatformWebBrowser.h>
#	include <IOS/ChromiumIOSCookieManager.h>
#elif PLATFORM_PS4
#	include "PS4PlatformWebBrowser.h"
#endif

// Define some platform-dependent file locations
#if WITH_CEF3
#	define CEF3_BIN_DIR TEXT("Binaries/ThirdParty/CEF3")
#	if PLATFORM_WINDOWS && PLATFORM_64BITS
#		define CEF3_RESOURCES_DIR CEF3_BIN_DIR TEXT("/Win64/Resources")
#		define CEF3_SUBPROCES_EXE TEXT("Binaries/ThirdParty/CEF3/Win64/UnrealCEFSubProcess.exe")
#	elif PLATFORM_WINDOWS && PLATFORM_32BITS
#		define CEF3_RESOURCES_DIR CEF3_BIN_DIR TEXT("/Win32/Resources")
#		define CEF3_SUBPROCES_EXE TEXT("Binaries/Win32/UnrealCEFSubProcess.exe")
#	elif PLATFORM_MAC
#		define CEF3_FRAMEWORK_DIR CEF3_BIN_DIR TEXT("/Mac/Chromium Embedded Framework.framework")
#		define CEF3_RESOURCES_DIR CEF3_FRAMEWORK_DIR TEXT("/Resources")
#		define CEF3_SUBPROCES_EXE TEXT("Binaries/Mac/UnrealCEFSubProcess.app/Contents/MacOS/UnrealCEFSubProcess")
#	elif PLATFORM_LINUX // @todo Linux
#		define CEF3_RESOURCES_DIR CEF3_BIN_DIR TEXT("/Linux/Resources")
#		define CEF3_SUBPROCES_EXE TEXT("Binaries/Linux/UnrealCEFSubProcess")
#	endif
	// Caching is enabled by default.
#	ifndef CEF3_DEFAULT_CACHE
#		define CEF3_DEFAULT_CACHE 1
#	endif
#endif

namespace {

	/**
	 * Helper function to set the current thread name, visible by the debugger.
	 * @param ThreadName	Name to set
	 */
	void SetCurrentThreadName(char* ThreadName)
	{
#if PLATFORM_MAC
		pthread_setname_np(ThreadName);
#elif PLATFORM_LINUX
		pthread_setname_np(pthread_self(), ThreadName);
#elif PLATFORM_WINDOWS && !PLATFORM_SEH_EXCEPTIONS_DISABLED
		/**
		 * Code setting the thread name for use in the debugger.
		 * Copied implementation from WindowsRunnableThread as it is private.
		 *
		 * http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
		 */
		const uint32 MS_VC_EXCEPTION=0x406D1388;

		struct THREADNAME_INFO
		{
			uint32 dwType;		// Must be 0x1000.
			LPCSTR szName;		// Pointer to name (in user addr space).
			uint32 dwThreadID;	// Thread ID (-1=caller thread).
			uint32 dwFlags;		// Reserved for future use, must be zero.
		};

		THREADNAME_INFO ThreadNameInfo = {0x1000, ThreadName, (uint32)-1, 0};

		__try
		{
			RaiseException( MS_VC_EXCEPTION, 0, sizeof(ThreadNameInfo)/sizeof(ULONG_PTR), (ULONG_PTR*)&ThreadNameInfo );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		CA_SUPPRESS(6322)
		{
		}
#endif
	}
}

FString FChromiumWebBrowserSingleton::ApplicationCacheDir() const
{
#if PLATFORM_MAC
	// OSX wants caches in a separate location from other app data
	static TCHAR Result[MAC_MAX_PATH] = TEXT("");
	if (!Result[0])
	{
		SCOPED_AUTORELEASE_POOL;
		NSString *CacheBaseDir = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
		NSString* BundleID = [[NSBundle mainBundle] bundleIdentifier];
		if(!BundleID)
		{
			BundleID = [[NSProcessInfo processInfo] processName];
		}
		check(BundleID);

		NSString* AppCacheDir = [CacheBaseDir stringByAppendingPathComponent: BundleID];
		FPlatformString::CFStringToTCHAR((CFStringRef)AppCacheDir, Result);
	}
	return FString(Result);
#else
	// Other platforms use the application data directory
	return FPaths::ProjectSavedDir();
#endif
}


PRAGMA_DISABLE_DEPRECATION_WARNINGS
class FChromiumWebBrowserWindowFactory
	: public IChromiumWebBrowserWindowFactory
{
public:

	virtual ~FChromiumWebBrowserWindowFactory()
	{ }

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo) override
	{
		return IChromiumWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow(
			BrowserWindowParent,
			BrowserWindowInfo);
	}

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		void* OSWindowHandle,
		FString InitialURL,
		bool bUseTransparency,
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(),
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255)) override
	{
		return IChromiumWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow(
			OSWindowHandle,
			InitialURL,
			bUseTransparency,
			bThumbMouseButtonNavigation,
			ContentsToLoad,
			ShowErrorMessage,
			BackgroundColor);
	}
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS

class FChromiumNoWebBrowserWindowFactory
	: public IChromiumWebBrowserWindowFactory
{
public:

	virtual ~FChromiumNoWebBrowserWindowFactory()
	{ }

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo) override
	{
		return nullptr;
	}

	virtual TSharedPtr<IChromiumWebBrowserWindow> Create(
		void* OSWindowHandle,
		FString InitialURL,
		bool bUseTransparency,
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(),
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255)) override
	{
		return nullptr;
	}
};

FChromiumWebBrowserSingleton::FChromiumWebBrowserSingleton(const FChromiumWebBrowserInitSettings& WebBrowserInitSettings)
#if WITH_CEF3
	: WebBrowserWindowFactory(MakeShareable(new FChromiumWebBrowserWindowFactory()))
#else
	: WebBrowserWindowFactory(MakeShareable(new FChromiumNoWebBrowserWindowFactory()))
#endif
	, bDevToolsShortcutEnabled(UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
	, bJSBindingsToLoweringEnabled(true)
	, DefaultMaterial(nullptr)
	, DefaultTranslucentMaterial(nullptr)
{
#if WITH_CEF3
	// The FChromiumWebBrowserSingleton must be initialized on the game thread
	check(IsInGameThread());

	// Provide CEF with command-line arguments.
#if PLATFORM_WINDOWS
	CefMainArgs MainArgs(hInstance);
#else
	CefMainArgs MainArgs;
#endif

	// Enable high-DPI support early in CEF startup. For this to work it also depends
	//  on FPlatformApplicationMisc::SetHighDPIMode() being called already which should happen by default
	//CefEnableHighDPISupport();

	bool bVerboseLogging = FParse::Param(FCommandLine::Get(), TEXT("cefverbose")) || FParse::Param(FCommandLine::Get(), TEXT("debuglog"));
	// CEFBrowserApp implements application-level callbacks.
	CEFBrowserApp = new FChromiumCEFBrowserApp;
	//CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
	//command_line->AppendSwitchWithValue("ppapi-flash-path", "pepflashplayer64_29_0_0_171.dll");
	//command_line->AppendSwitchWithValue("ppapi-flash-version", "29.0.0.171");
	//command_line->AppendSwitchWithValue("plugin-policy", "allow");
	//command_line->AppendSwitchWithValue("enable-system-flash", "1");
	//FCommandLine::Append(TEXT(" --enable-system-flash=1 --plugin-policy=allow "));
	CEFBrowserApp->OnRenderProcessThreadCreated().BindRaw(this, &FChromiumWebBrowserSingleton::HandleRenderProcessCreated);
	// Specify CEF global settings here.
	CefSettings Settings;
	Settings.no_sandbox = true;
	Settings.command_line_args_disabled = true;
	Settings.external_message_pump = true;
	//@todo change to threaded version instead of using external_message_pump & OnScheduleMessagePumpWork
	Settings.multi_threaded_message_loop = false;
	//Set the default background for browsers to be opaque black, this is used for windowed (not OSR) browsers
	//  setting it black here prevents the white flash on load
	Settings.background_color = CefColorSetARGB(255, 0, 0, 0);
	

	FString CefLogFile(FPaths::Combine(*FPaths::ProjectLogDir(), TEXT("cef3.log")));
	CefLogFile = FPaths::ConvertRelativePathToFull(CefLogFile);
	CefString(&Settings.log_file) = TCHAR_TO_WCHAR(*CefLogFile);
	Settings.log_severity = bVerboseLogging ? LOGSEVERITY_VERBOSE : LOGSEVERITY_WARNING;

	uint16 DebugPort;
	if(FParse::Value(FCommandLine::Get(), TEXT("cefdebug="), DebugPort))
	{
		Settings.remote_debugging_port = DebugPort;
	}

	// Specify locale from our settings
	FString LocaleCode = GetCurrentLocaleCode();
	CefString(&Settings.locale) = TCHAR_TO_WCHAR(*LocaleCode);

	// Append engine version to the user agent string.
	CefString(&Settings.product_version) = TCHAR_TO_WCHAR(*WebBrowserInitSettings.ProductVersion);

#if CEF3_DEFAULT_CACHE
	// Enable on disk cache
	FString CachePath(FPaths::Combine(ApplicationCacheDir(), TEXT("webcache")));
	CachePath = FPaths::ConvertRelativePathToFull(GenerateWebCacheFolderName(CachePath));
	CefString(&Settings.cache_path) = TCHAR_TO_WCHAR(*CachePath);
#endif

	// Specify path to resources
	FString ResourcesPath(FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("ChromiumUI"), CEF3_RESOURCES_DIR));
	ResourcesPath = FPaths::ConvertRelativePathToFull(ResourcesPath);
	if (!FPaths::DirectoryExists(ResourcesPath))
	{
		UE_LOG(ChromiumLogWebBrowser, Error, TEXT("Chromium Resources information not found at: %s."), *ResourcesPath);
	}
	CefString(&Settings.resources_dir_path) = TCHAR_TO_WCHAR(*ResourcesPath);

#if !PLATFORM_MAC
	// On Mac Chromium ignores custom locales dir. Files need to be stored in Resources folder in the app bundle
	FString LocalesPath(FPaths::Combine(*ResourcesPath, TEXT("locales")));
	LocalesPath = FPaths::ConvertRelativePathToFull(LocalesPath);
	if (!FPaths::DirectoryExists(LocalesPath))
	{
		UE_LOG(ChromiumLogWebBrowser, Error, TEXT("Chromium Locales information not found at: %s."), *LocalesPath);
	}
	CefString(&Settings.locales_dir_path) = TCHAR_TO_WCHAR(*LocalesPath);
#else
	// LocaleCode may contain region, which for some languages may make CEF unable to find the locale pak files
	// In that case use the language name for CEF locale
	FString LocalePakPath = ResourcesPath + TEXT("/") + LocaleCode.Replace(TEXT("-"), TEXT("_")) + TEXT(".lproj/locale.pak");
	if (!FPaths::FileExists(LocalePakPath))
	{
		FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
		LocaleCode = Culture->GetTwoLetterISOLanguageName();
		LocalePakPath = ResourcesPath + TEXT("/") + LocaleCode + TEXT(".lproj/locale.pak");
		if (FPaths::FileExists(LocalePakPath))
		{
			CefString(&Settings.locale) = TCHAR_TO_WCHAR(*LocaleCode);
		}
	}
	
	// Let CEF know where we have put the framework bundle as it is non-default
	FString CefFrameworkPath(FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("ChromiumUI"), CEF3_FRAMEWORK_DIR));
	CefFrameworkPath = FPaths::ConvertRelativePathToFull(CefFrameworkPath);
	CefString(&Settings.framework_dir_path) = TCHAR_TO_WCHAR(*CefFrameworkPath);
	CefString(&Settings.main_bundle_path) = TCHAR_TO_WCHAR(*CefFrameworkPath);
#endif

	// Specify path to sub process exe
	FString SubProcessPath(FPaths::Combine(*FPaths::ProjectPluginsDir(),TEXT("ChromiumUI"), CEF3_SUBPROCES_EXE));
	SubProcessPath = FPaths::ConvertRelativePathToFull(SubProcessPath);

	if (!IPlatformFile::GetPlatformPhysical().FileExists(*SubProcessPath))
	{
		UE_LOG(ChromiumLogWebBrowser, Error, TEXT("UnrealCEFSubProcess.exe not found, check that this program has been built and is placed in: %s."), *SubProcessPath);
	}
	CefString(&Settings.browser_subprocess_path) = TCHAR_TO_WCHAR(*SubProcessPath);

	// Initialize CEF.
	bool bSuccess = CefInitialize(MainArgs, Settings, CEFBrowserApp.get(), nullptr);
	check(bSuccess);

	// Set the thread name back to GameThread.
	SetCurrentThreadName(TCHAR_TO_ANSI( *(FName( NAME_GameThread ).GetPlainNameString()) ));

	DefaultCookieManager = FChromiumCefWebBrowserCookieManagerFactory::Create(CefCookieManager::GetGlobalManager(nullptr));
#elif PLATFORM_IOS && !BUILD_EMBEDDED_APP
	DefaultCookieManager = MakeShareable(new FChromiumIOSCookieManager());
#elif PLATFORM_ANDROID
	DefaultCookieManager = MakeShareable(new FChromiumAndroidCookieManager());
#endif
}

#if WITH_CEF3
void FChromiumWebBrowserSingleton::HandleRenderProcessCreated(CefRefPtr<CefListValue> ExtraInfo)
{
	FScopeLock Lock(&WindowInterfacesCS);
	for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
	{
		TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
		if (BrowserWindow.IsValid())
		{
			CefRefPtr<CefDictionaryValue> Bindings = BrowserWindow->GetProcessInfo();
			if (Bindings.get())
			{
				ExtraInfo->SetDictionary(ExtraInfo->GetSize(), Bindings);
			}
		}
	}
}
#endif

FChromiumWebBrowserSingleton::~FChromiumWebBrowserSingleton()
{
#if WITH_CEF3
	{
		FScopeLock Lock(&WindowInterfacesCS);
		// Force all existing browsers to close in case any haven't been deleted
		for (int32 Index = 0; Index < WindowInterfaces.Num(); ++Index)
		{
			auto BrowserWindow = WindowInterfaces[Index].Pin();
			if (BrowserWindow.IsValid() && BrowserWindow->IsValid())
			{
				// Call CloseBrowser directly on the Host object as FWebBrowserWindow::CloseBrowser is delayed
				BrowserWindow->InternalCefBrowser->GetHost()->CloseBrowser(true);
			}
		}
		// Clear this before CefShutdown() below
		WindowInterfaces.Reset();
	}

	// Remove references to the scheme handler factories
	CefClearSchemeHandlerFactories();
	for (const TPair<FString, CefRefPtr<CefRequestContext>>& RequestContextPair : RequestContexts)
	{
		RequestContextPair.Value->ClearSchemeHandlerFactories();
	}
	// Clear this before CefShutdown() below
	RequestContexts.Reset();

	// make sure any handler before load delegates are unbound
	for (const TPair <FString,CefRefPtr<FChromiumCEFResourceContextHandler>>& HandlerPair : RequestResourceHandlers)
	{
		HandlerPair.Value->OnBeforeLoad().Unbind();
	}
	// Clear this before CefShutdown() below
	RequestResourceHandlers.Reset();
	// Just in case, although we deallocate CEFBrowserApp right after this.
	CEFBrowserApp->OnRenderProcessThreadCreated().Unbind();
	// CefRefPtr takes care of delete
	CEFBrowserApp = nullptr;
	// Ensure we run the message pump
	CefDoMessageLoopWork();

	// Keep pumping messages until we see the one below clear the queue
	bTaskFinished = false;
	CefPostTask(TID_UI, new FChromiumCEFBrowserClosureTask(nullptr, [=]()
		{
			bTaskFinished = true;
		}));

	const double StartWaitAppTime = FPlatformTime::Seconds();
	while (!bTaskFinished)
	{
		CefDoMessageLoopWork();
		// Wait at most 1 second for tasks to clear, in case CEF crashes/hangs during process lifetime
		if (FPlatformTime::Seconds() - StartWaitAppTime > 1.0f)
		{
			break; // don't spin forever
		}
	}
	
	// Shut down CEF.
	CefShutdown();
#elif PLATFORM_IOS || PLATFORM_PS4 || (PLATFORM_ANDROID && USE_ANDROID_JNI)
	{
		FScopeLock Lock(&WindowInterfacesCS);
		// Clear this before CefShutdown() below
		WindowInterfaces.Reset();
	}
#endif
}

TSharedRef<IChromiumWebBrowserWindowFactory> FChromiumWebBrowserSingleton::GetWebBrowserWindowFactory() const
{
	return WebBrowserWindowFactory;
}

TSharedPtr<IChromiumWebBrowserWindow> FChromiumWebBrowserSingleton::CreateBrowserWindow(
	TSharedPtr<FChromiumCEFWebBrowserWindow>& BrowserWindowParent,
	TSharedPtr<FChromiumWebBrowserWindowInfo>& BrowserWindowInfo
	)
{
#if WITH_CEF3

	TOptional<FString> ContentsToLoad;

	bool bShowErrorMessage = BrowserWindowParent->IsShowingErrorMessages();
	bool bThumbMouseButtonNavigation = BrowserWindowParent->IsThumbMouseButtonNavigationEnabled();
	bool bUseTransparency = BrowserWindowParent->UseTransparency();
	bool bUsingAcceleratedPaint = BrowserWindowParent->UsingAcceleratedPaint();
	FString InitialURL = WCHAR_TO_TCHAR(BrowserWindowInfo->Browser->GetMainFrame()->GetURL().ToWString().c_str());
	TSharedPtr<FChromiumCEFWebBrowserWindow> NewBrowserWindow(new FChromiumCEFWebBrowserWindow(BrowserWindowInfo->Browser, BrowserWindowInfo->Handler, InitialURL, ContentsToLoad, bShowErrorMessage, bThumbMouseButtonNavigation, bUseTransparency, bJSBindingsToLoweringEnabled, bUsingAcceleratedPaint));
	BrowserWindowInfo->Handler->SetBrowserWindow(NewBrowserWindow);
	{
		FScopeLock Lock(&WindowInterfacesCS);
		WindowInterfaces.Add(NewBrowserWindow);
	}
	NewBrowserWindow->GetCefBrowser()->GetHost()->SetWindowlessFrameRate(BrowserWindowParent->GetCefBrowser()->GetHost()->GetWindowlessFrameRate());
	return NewBrowserWindow;
#endif
	return nullptr;
}

TSharedPtr<IChromiumWebBrowserWindow> FChromiumWebBrowserSingleton::CreateBrowserWindow(
	void* OSWindowHandle,
	FString InitialURL,
	bool bUseTransparency,
	bool bThumbMouseButtonNavigation,
	TOptional<FString> ContentsToLoad,
	bool ShowErrorMessage,
	FColor BackgroundColor,
	int BrowserFrameRate,
	const TArray<FString>& AltRetryDomains)
{
	FChromiumCreateBrowserWindowSettings Settings;
	Settings.OSWindowHandle = OSWindowHandle;
	Settings.InitialURL = InitialURL;
	Settings.bUseTransparency = false;// bUseTransparency;
	Settings.bThumbMouseButtonNavigation = bThumbMouseButtonNavigation;
	Settings.ContentsToLoad = ContentsToLoad;
	Settings.bShowErrorMessage = ShowErrorMessage;
	Settings.BackgroundColor = FColor::Black;// BackgroundColor;
	Settings.BrowserFrameRate = BrowserFrameRate;
	Settings.AltRetryDomains = AltRetryDomains;

	return CreateBrowserWindow(Settings);
}

TSharedPtr<IChromiumWebBrowserWindow> FChromiumWebBrowserSingleton::CreateBrowserWindow(const FChromiumCreateBrowserWindowSettings& WindowSettings)
{
	bool bBrowserEnabled = true;
	GConfig->GetBool(TEXT("Browser"), TEXT("bEnabled"), bBrowserEnabled, GEngineIni);
	if (!bBrowserEnabled || !FApp::CanEverRender())
	{
		return nullptr;
	}

#if WITH_CEF3
	static bool AllowCEF = !FParse::Param(FCommandLine::Get(), TEXT("nocef"));
	if (AllowCEF)
	{
		// Information used when creating the native window.
		CefWindowInfo WindowInfo;

		// Specify CEF browser settings here.
		CefBrowserSettings BrowserSettings;

		// The color to paint before a document is loaded
		// if using a windowed(native) browser window AND bUseTransparency is true then the background actually uses Settings.background_color from above
		// if using a OSR window and bUseTransparency is true then you get a transparency channel in your BGRA OnPaint
		// if bUseTransparency is false then you get the background color defined by your RGB setting here
		BrowserSettings.background_color = CefColorSetARGB(WindowSettings.bUseTransparency ? 0 : WindowSettings.BackgroundColor.A, WindowSettings.BackgroundColor.R, WindowSettings.BackgroundColor.G, WindowSettings.BackgroundColor.B);

		// Disable plugins
		BrowserSettings.plugins = STATE_DISABLED;


#if PLATFORM_WINDOWS
		// Create the widget as a child window on windows when passing in a parent window
		if (WindowSettings.OSWindowHandle != nullptr)
		{
			RECT ClientRect = { 0, 0, 0, 0 };
			WindowInfo.SetAsChild((CefWindowHandle)WindowSettings.OSWindowHandle, ClientRect);
		}
		else
#endif
		{
			// Use off screen rendering so we can integrate with our windows
			WindowInfo.SetAsWindowless(kNullWindowHandle);
			WindowInfo.shared_texture_enabled = FChromiumCEFWebBrowserWindow::CanSupportAcceleratedPaint() ? 1 : 0;
			int BrowserFrameRate = WindowSettings.BrowserFrameRate;
			if (FChromiumCEFWebBrowserWindow::CanSupportAcceleratedPaint() && BrowserFrameRate == 24)
			{
				// Use 60 fps if the accelerated renderer is enabled and the default framerate was otherwise selected
				BrowserFrameRate = 60;
			}
			BrowserSettings.windowless_frame_rate = BrowserFrameRate;
		}


		// WebBrowserHandler implements browser-level callbacks.
		CefRefPtr<FChromiumCEFBrowserHandler> NewHandler(new FChromiumCEFBrowserHandler(WindowSettings.bUseTransparency, WindowSettings.AltRetryDomains));

		CefRefPtr<CefRequestContext> RequestContext = nullptr;
		if (WindowSettings.Context.IsSet())
		{
			const FChromiumBrowserContextSettings Context = WindowSettings.Context.GetValue();
			const CefRefPtr<CefRequestContext>* ExistingRequestContext = RequestContexts.Find(Context.Id);

			if (ExistingRequestContext == nullptr)
			{
				CefRequestContextSettings RequestContextSettings;
				CefString(&RequestContextSettings.accept_language_list) = Context.AcceptLanguageList.IsEmpty() ? TCHAR_TO_WCHAR(*GetCurrentLocaleCode()) : TCHAR_TO_WCHAR(*Context.AcceptLanguageList);
				CefString(&RequestContextSettings.cache_path) = TCHAR_TO_WCHAR(*GenerateWebCacheFolderName(Context.CookieStorageLocation));
				RequestContextSettings.persist_session_cookies = Context.bPersistSessionCookies;
				RequestContextSettings.ignore_certificate_errors = Context.bIgnoreCertificateErrors;

				CefRefPtr<FChromiumCEFResourceContextHandler> ResourceContextHandler = new FChromiumCEFResourceContextHandler();
				ResourceContextHandler->OnBeforeLoad() = Context.OnBeforeContextResourceLoad;
				RequestResourceHandlers.Add(Context.Id, ResourceContextHandler);

				//Create a new one
				RequestContext = CefRequestContext::CreateContext(RequestContextSettings, ResourceContextHandler);
				RequestContexts.Add(Context.Id, RequestContext);
			}
			else
			{
				RequestContext = *ExistingRequestContext;
			}
			SchemeHandlerFactories.RegisterFactoriesWith(RequestContext);
		}

		// Create the CEF browser window.
		CefRefPtr<CefBrowser> Browser = CefBrowserHost::CreateBrowserSync(WindowInfo, NewHandler.get(), TCHAR_TO_WCHAR(*WindowSettings.InitialURL), BrowserSettings, nullptr, RequestContext);
		if (Browser.get())
		{
			// Create new window
			TSharedPtr<FChromiumCEFWebBrowserWindow> NewBrowserWindow = MakeShareable(new FChromiumCEFWebBrowserWindow(
				Browser,
				NewHandler,
				WindowSettings.InitialURL,
				WindowSettings.ContentsToLoad,
				WindowSettings.bShowErrorMessage,
				WindowSettings.bThumbMouseButtonNavigation,
				WindowSettings.bUseTransparency,
				bJSBindingsToLoweringEnabled,
				false));
			NewHandler->SetBrowserWindow(NewBrowserWindow);
			{
				FScopeLock Lock(&WindowInterfacesCS);
				WindowInterfaces.Add(NewBrowserWindow);
			}

			return NewBrowserWindow;
		}
	}
#elif PLATFORM_ANDROID && USE_ANDROID_JNI
	// Create new window
	TSharedPtr<FChromiumAndroidWebBrowserWindow> NewBrowserWindow = MakeShareable(new FChromiumAndroidWebBrowserWindow(
		WindowSettings.InitialURL,
		WindowSettings.ContentsToLoad,
		WindowSettings.bShowErrorMessage,
		WindowSettings.bThumbMouseButtonNavigation,
		WindowSettings.bUseTransparency,
		bJSBindingsToLoweringEnabled));

	{
		FScopeLock Lock(&WindowInterfacesCS);
		WindowInterfaces.Add(NewBrowserWindow);
	}
	return NewBrowserWindow;
#elif PLATFORM_IOS
	// Create new window
	TSharedPtr<FChromiumWebBrowserWindow> NewBrowserWindow = MakeShareable(new FChromiumWebBrowserWindow(
		WindowSettings.InitialURL, 
		WindowSettings.ContentsToLoad, 
		WindowSettings.bShowErrorMessage, 
		WindowSettings.bThumbMouseButtonNavigation, 
		WindowSettings.bUseTransparency,
		bJSBindingsToLoweringEnabled));

	{
		FScopeLock Lock(&WindowInterfacesCS);
		WindowInterfaces.Add(NewBrowserWindow);
	}
	return NewBrowserWindow;
#elif PLATFORM_PS4
	// Create new window
	TSharedPtr<FChromiumWebBrowserWindow> NewBrowserWindow = MakeShareable(new FChromiumWebBrowserWindow(
		WindowSettings.InitialURL,
		WindowSettings.ContentsToLoad,
		WindowSettings.bShowErrorMessage,
		WindowSettings.bThumbMouseButtonNavigation,
		WindowSettings.bUseTransparency));

	{
		FScopeLock Lock(&WindowInterfacesCS);
		WindowInterfaces.Add(NewBrowserWindow);
	}
	return NewBrowserWindow;
#endif
	return nullptr;
}

#if BUILD_EMBEDDED_APP
TSharedPtr<IChromiumWebBrowserWindow> FChromiumWebBrowserSingleton::CreateNativeBrowserProxy()
{
	TSharedPtr<FNativeWebBrowserProxy> NewBrowserWindow = MakeShareable(new FNativeWebBrowserProxy(
		bJSBindingsToLoweringEnabled
	));
	NewBrowserWindow->Initialize();
	return NewBrowserWindow;
}
#endif //BUILD_EMBEDDED_APP

bool FChromiumWebBrowserSingleton::Tick(float DeltaTime)
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_FChromiumWebBrowserSingleton_Tick);

#if WITH_CEF3
	{
		FScopeLock Lock(&WindowInterfacesCS);
		bool bIsSlateAwake = FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsSlateAsleep();
		// Remove any windows that have been deleted and check whether it's currently visible
		for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
		{
			if (!WindowInterfaces[Index].IsValid())
			{
				WindowInterfaces.RemoveAt(Index);
			}
			else if (bIsSlateAwake) // only check for Tick activity if Slate is currently ticking
			{
				TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
				if(BrowserWindow.IsValid())
				{
					// Test if we've ticked recently. If not assume the browser window has become hidden.
					BrowserWindow->CheckTickActivity();
				}
			}
		}
	}

	if (CEFBrowserApp != nullptr)
	{
		bool bForceMessageLoop = false;
		GConfig->GetBool(TEXT("Browser"), TEXT("bForceMessageLoop"), bForceMessageLoop, GEngineIni);

		// Get the configured minimum hertz and make sure the value is within a reasonable range
		int32 MinMessageLoopHz = 1;
		GConfig->GetInt(TEXT("Browser"), TEXT("MinMessageLoopHertz"), MinMessageLoopHz, GEngineIni);
		MinMessageLoopHz = FMath::Clamp(MinMessageLoopHz, 1, 60);

		// Get the configured forced maximum hertz and make sure the value is within a reasonable range
		int32 MaxForcedMessageLoopHz = 15;
		GConfig->GetInt(TEXT("Browser"), TEXT("MaxForcedMessageLoopHertz"), MaxForcedMessageLoopHz, GEngineIni);
		MaxForcedMessageLoopHz = FMath::Clamp(MaxForcedMessageLoopHz, MinMessageLoopHz, 60);

		// @todo: Hack: We rely on OnScheduleMessagePumpWork() which tells us to drive the CEF message pump, 
		//  there appear to be some edge cases where we might not be getting a signal from it so for the time being 
		//  we force a minimum rates here and let it run at a configurable maximum rate when we have any WindowInterfaces.

		// Convert to seconds which we'll use to compare against the time we accumulated since last pump / left till next pump
		float MinMessageLoopSeconds = 1.0f / MinMessageLoopHz;
		float MaxForcedMessageLoopSeconds = 1.0f / MaxForcedMessageLoopHz;

		static float SecondsSinceLastPump = 0;
		static float SecondsToNextForcedPump = MaxForcedMessageLoopSeconds;

		// Accumulate time since last pump by adding DeltaTime which gives us the amount of time that has passed since last tick in seconds
		SecondsSinceLastPump += DeltaTime;
		// Time left till next pump
		SecondsToNextForcedPump -= DeltaTime;

		bool bWantForce = bForceMessageLoop || WindowInterfaces.Num() > 0;    // True if we wish to force message pump
		bool bCanForce = SecondsToNextForcedPump <= 0;                        // But can we?
		bool bMustForce = SecondsSinceLastPump >= MinMessageLoopSeconds;      // Absolutely must force (Min frequency rate hit)

		// tick the CEF app to determine when to run CefDoMessageLoopWork
		if (CEFBrowserApp->TickMessagePump(DeltaTime, (bWantForce && bCanForce) || bMustForce))
		{
			SecondsSinceLastPump = 0;
			SecondsToNextForcedPump = MaxForcedMessageLoopSeconds;
		}
	}

	// Update video buffering for any windows that need it
	for (int32 Index = 0; Index < WindowInterfaces.Num(); Index++)
	{
		if (WindowInterfaces[Index].IsValid())
		{
			TSharedPtr<FChromiumCEFWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
			if (BrowserWindow.IsValid())
			{
				BrowserWindow->UpdateVideoBuffering();
			}
		}
	}

#elif PLATFORM_IOS || PLATFORM_PS4 || (PLATFORM_ANDROID && USE_ANDROID_JNI)
	FScopeLock Lock(&WindowInterfacesCS);
	bool bIsSlateAwake = FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsSlateAsleep();
	// Remove any windows that have been deleted and check whether it's currently visible
	for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
	{
		if (!WindowInterfaces[Index].IsValid())
		{
			WindowInterfaces.RemoveAt(Index);
		}
		else if (bIsSlateAwake) // only check for Tick activity if Slate is currently ticking
		{
			TSharedPtr<IChromiumWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
			if (BrowserWindow.IsValid())
			{
				// Test if we've ticked recently. If not assume the browser window has become hidden.
				BrowserWindow->CheckTickActivity();
			}
		}
	}

#endif
	return true;
}

FString FChromiumWebBrowserSingleton::GetCurrentLocaleCode()
{
	FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
	FString LocaleCode = Culture->GetTwoLetterISOLanguageName();
	FString Country = Culture->GetRegion();
	if (!Country.IsEmpty())
	{
		LocaleCode = LocaleCode + TEXT("-") + Country;
	}
	return LocaleCode;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void FChromiumWebBrowserSingleton::DeleteBrowserCookies(FString URL, FString CookieName, TFunction<void(int)> Completed)
{
	if (DefaultCookieManager.IsValid())
	{
		DefaultCookieManager->DeleteCookies(URL, CookieName, Completed);
	}
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

TSharedPtr<IChromiumWebBrowserCookieManager> FChromiumWebBrowserSingleton::GetCookieManager(TOptional<FString> ContextId) const
{
	if (ContextId.IsSet())
	{
#if WITH_CEF3
		const CefRefPtr<CefRequestContext>* ExistingContext = RequestContexts.Find(ContextId.GetValue());

		if (ExistingContext && ExistingContext->get())
		{
			// Cache these cookie managers?
			return FChromiumCefWebBrowserCookieManagerFactory::Create((*ExistingContext)->GetCookieManager(nullptr));
		}
		else
		{
			UE_LOG(ChromiumLogWebBrowser, Log, TEXT("No cookie manager for ContextId=%s.  Using default cookie manager"), *ContextId.GetValue());
		}
#endif
	}
	// No ContextId or cookie manager instance associated with it.  Use default
	return DefaultCookieManager;
}

#if WITH_CEF3
FString FChromiumWebBrowserSingleton::GenerateWebCacheFolderName(const FString& InputPath)
{
	if (InputPath.IsEmpty())
		return InputPath;

	// append the version of this CEF build to our requested cache folder path
	// this means each new CEF build gets its own cache folder, making downgrading safe
	return InputPath + "_" + MAKE_STRING(CHROME_VERSION_BUILD);
}
#endif

void FChromiumWebBrowserSingleton::ClearOldCacheFolders(const FString &CachePathRoot, const FString &CachePrefix)
{
#if WITH_CEF3
	/*
	// only CEF3 currently has version dependant cache folders that may need cleanup
	struct FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		const FString CachePrefix;
		const FString CurrentCachePath;

		FDirectoryVisitor(const FString &InCachePrefix, const FString &InCurrentCachePath)
			: CachePrefix(InCachePrefix),
			CurrentCachePath(InCurrentCachePath)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			static const FString CachePrefixSearch = "/" + CachePrefix;
			if (bIsDirectory)
			{
				FString DirName(FilenameOrDirectory);
				if (DirName.Contains(CachePrefixSearch) && DirName.Equals(CurrentCachePath)==false)
				{
					UE_LOG(ChromiumLogWebBrowser, Log, TEXT("Old Cache folder found=%s, deleting"), *DirName);
					// BUGBUG - enable this deletion once we are happy with the new CEF version rollout
					// Also consider adding code to preserve the previous versions folder for a while?

				}
			}
			
			return true;
		}
	};

	// Enumerate the contents of the current directory
	FDirectoryVisitor Visitor(CachePrefix, GenerateWebCacheFolderName(FPaths::Combine(CachePathRoot, CachePrefix)));
	IPlatformFile::GetPlatformPhysical().IterateDirectory(*CachePathRoot, Visitor);

	*/
#endif
	
}

bool FChromiumWebBrowserSingleton::RegisterContext(const FChromiumBrowserContextSettings& Settings)
{
#if WITH_CEF3
	const CefRefPtr<CefRequestContext>* ExistingContext = RequestContexts.Find(Settings.Id);

	if (ExistingContext != nullptr)
	{
		// You can't register the same context twice and
		// you can't update the settings for a context that already exists
		return false;
	}

	CefRequestContextSettings RequestContextSettings;
	CefString(&RequestContextSettings.accept_language_list) = Settings.AcceptLanguageList.IsEmpty() ? TCHAR_TO_WCHAR(*GetCurrentLocaleCode()) : TCHAR_TO_WCHAR(*Settings.AcceptLanguageList);
	CefString(&RequestContextSettings.cache_path) = TCHAR_TO_WCHAR(*GenerateWebCacheFolderName(Settings.CookieStorageLocation));
	RequestContextSettings.persist_session_cookies = Settings.bPersistSessionCookies;
	RequestContextSettings.ignore_certificate_errors = Settings.bIgnoreCertificateErrors;

	//Create a new one
	CefRefPtr<FChromiumCEFResourceContextHandler> ResourceContextHandler = new FChromiumCEFResourceContextHandler();
	ResourceContextHandler->OnBeforeLoad() = Settings.OnBeforeContextResourceLoad;
	RequestResourceHandlers.Add(Settings.Id, ResourceContextHandler);
	CefRefPtr<CefRequestContext> RequestContext = CefRequestContext::CreateContext(RequestContextSettings, ResourceContextHandler);
	RequestContexts.Add(Settings.Id, RequestContext);
	return true;
#else
	return false;
#endif
}

bool FChromiumWebBrowserSingleton::UnregisterContext(const FString& ContextId)
{
	bool bFoundContext = false;
#if WITH_CEF3
	//bool bFoundContext = false;
	CefRefPtr<CefRequestContext> Context;
	if (RequestContexts.RemoveAndCopyValue(ContextId, Context))
	{
		bFoundContext = true;
		Context->ClearSchemeHandlerFactories();
	}

	CefRefPtr<FChromiumCEFResourceContextHandler> ResourceHandler;
	if (RequestResourceHandlers.RemoveAndCopyValue(ContextId, ResourceHandler))
	{
		ResourceHandler->OnBeforeLoad().Unbind();
	}

	return bFoundContext;
#else
	return false;
#endif
	return bFoundContext;
}

bool FChromiumWebBrowserSingleton::RegisterSchemeHandlerFactory(FString Scheme, FString Domain, IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory)
{
#if WITH_CEF3
	SchemeHandlerFactories.AddSchemeHandlerFactory(MoveTemp(Scheme), MoveTemp(Domain), WebBrowserSchemeHandlerFactory);
	return true;
#else
	return false;
#endif
}

bool FChromiumWebBrowserSingleton::UnregisterSchemeHandlerFactory(IChromiumWebBrowserSchemeHandlerFactory* WebBrowserSchemeHandlerFactory)
{
#if WITH_CEF3
	SchemeHandlerFactories.RemoveSchemeHandlerFactory(WebBrowserSchemeHandlerFactory);
	return true;
#else
	return false;
#endif
}

// Cleanup macros to avoid having them leak outside this source file
#undef CEF3_BIN_DIR
#undef CEF3_FRAMEWORK_DIR
#undef CEF3_RESOURCES_DIR
#undef CEF3_SUBPROCES_EXE
