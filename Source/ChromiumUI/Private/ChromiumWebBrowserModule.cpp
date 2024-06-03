// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumWebBrowserModule.h"
#include "ChromiumWebBrowserLog.h"
#include "ChromiumWebBrowserSingleton.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
//#if WITH_CEF3
//#	include "CEF3Utils.h"
//#endif
DEFINE_LOG_CATEGORY(ChromiumLogWebBrowser);

#if WITH_CEF3
#if PLATFORM_WINDOWS
	void* CEF3DLLHandle = nullptr;
	void* ElfHandle = nullptr;
	void* D3DHandle = nullptr;
	void* GLESHandle = nullptr;
	void* EGLHandle = nullptr;
#endif

	void* LoadDllCEF(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return nullptr;
		}
		void* Handle = FPlatformProcess::GetDllHandle(*Path);
		if (!Handle)
		{
			int32 ErrorNum = FPlatformMisc::GetLastError();
			TCHAR ErrorMsg[1024];
			FPlatformMisc::GetSystemErrorMessage(ErrorMsg, 1024, ErrorNum);
			UE_LOG(ChromiumLogWebBrowser, Error, TEXT("Failed to get CEF3 DLL handle for %s: %s (%d)"), *Path, ErrorMsg, ErrorNum);
		}
		return Handle;
	}

	void LoadCEF3Modules()
	{
#if PLATFORM_WINDOWS
		UE_LOG(ChromiumLogWebBrowser, Log, TEXT("Start Loading CEF3Modules!"));

#if PLATFORM_64BITS
		FString DllPath(FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("ChromiumUI"), TEXT("Binaries/ThirdParty/CEF3/Win64")));
#else
		FString DllPath(FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("Binaries/ThirdParty/CEF3/Win32")));
#endif

		FPlatformProcess::PushDllDirectory(*DllPath);
		CEF3DLLHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("libcef.dll")));
		if (CEF3DLLHandle)
		{
			ElfHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("chrome_elf.dll")));

#if WINVER >= 0x600 // Different dll used pre-Vista
			D3DHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("d3dcompiler_47.dll")));
#else
			D3DHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("d3dcompiler_43.dll")));
#endif
			GLESHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("libGLESv2.dll")));
			EGLHandle = LoadDllCEF(FPaths::Combine(*DllPath, TEXT("libEGL.dll")));
		}
		UE_LOG(ChromiumLogWebBrowser, Log, TEXT("Loading All DLL"));

		FPlatformProcess::PopDllDirectory(*DllPath);

		UE_LOG(ChromiumLogWebBrowser, Log, TEXT("Loading DLL Success"));

#endif
	}

	void UnloadCEF3Modules()
	{
#if PLATFORM_WINDOWS
		FPlatformProcess::FreeDllHandle(CEF3DLLHandle);
		CEF3DLLHandle = nullptr;
		FPlatformProcess::FreeDllHandle(ElfHandle);
		ElfHandle = nullptr;
		FPlatformProcess::FreeDllHandle(D3DHandle);
		D3DHandle = nullptr;
		FPlatformProcess::FreeDllHandle(GLESHandle);
		GLESHandle = nullptr;
		FPlatformProcess::FreeDllHandle(EGLHandle);
		EGLHandle = nullptr;
#endif
	};
#endif //WITH_CEF3



static FChromiumWebBrowserSingleton* WebBrowserSingleton = nullptr;

FChromiumWebBrowserInitSettings::FChromiumWebBrowserInitSettings()
	: ProductVersion(FString::Printf(TEXT("%s/%s UnrealEngine/%s Chrome/84.0.4147.38"), FApp::GetProjectName(), FApp::GetBuildVersion(), *FEngineVersion::Current().ToString()))
{
}

class FChromiumUIModule : public IChromiumWebBrowserModule
{
private:
	// IModuleInterface Interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	virtual IChromiumWebBrowserSingleton* GetSingleton() override;
	virtual bool CustomInitialize(const FChromiumWebBrowserInitSettings& WebBrowserInitSettings) override;
};

void FChromiumUIModule::StartupModule()
{
	UE_LOG(ChromiumLogWebBrowser, Log, TEXT("Start CEF3Modules!"));
#if WITH_CEF3
	LoadCEF3Modules();
#endif
}

void FChromiumUIModule::ShutdownModule()
{
	if (WebBrowserSingleton != nullptr)
	{
		delete WebBrowserSingleton;
		WebBrowserSingleton = nullptr;
	}

#if WITH_CEF3
	UnloadCEF3Modules();
#endif
}

bool FChromiumUIModule::CustomInitialize(const FChromiumWebBrowserInitSettings& WebBrowserInitSettings)
{
	UE_LOG(ChromiumLogWebBrowser, Log, TEXT("CustomInitialize"));

	if (WebBrowserSingleton == nullptr)
	{
		WebBrowserSingleton = new FChromiumWebBrowserSingleton(WebBrowserInitSettings);
		return true;
	}
	return false;
}

IChromiumWebBrowserSingleton* FChromiumUIModule::GetSingleton()
{
	if (WebBrowserSingleton == nullptr)
	{
		WebBrowserSingleton = new FChromiumWebBrowserSingleton(FChromiumWebBrowserInitSettings());
	}
	return WebBrowserSingleton;
}


IMPLEMENT_MODULE(FChromiumUIModule, ChromiumUI)