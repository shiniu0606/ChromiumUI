// Copyright Epic Games, Inc. All Rights Reserved.

#include "WebBrowserModule.h"
#include "WebBrowserLog.h"
#include "WebBrowserSingleton.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
//#if WITH_CEF3
//#	include "CEF3Utils.h"
//#endif
DEFINE_LOG_CATEGORY(LogWebBrowser);

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
			UE_LOG(LogWebBrowser, Error, TEXT("Failed to get CEF3 DLL handle for %s: %s (%d)"), *Path, ErrorMsg, ErrorNum);
		}
		return Handle;
	}

	void LoadCEF3Modules()
	{
#if PLATFORM_WINDOWS
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
		FPlatformProcess::PopDllDirectory(*DllPath);
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



static FWebBrowserSingleton* WebBrowserSingleton = nullptr;

FWebBrowserInitSettings::FWebBrowserInitSettings()
	: ProductVersion(FString::Printf(TEXT("%s/%s UnrealEngine/%s Chrome/59.0.3071.15"), FApp::GetProjectName(), FApp::GetBuildVersion(), *FEngineVersion::Current().ToString()))
{
}

class FChromiumUIModule : public IWebBrowserModule
{
private:
	// IModuleInterface Interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	virtual IWebBrowserSingleton* GetSingleton() override;
	virtual bool CustomInitialize(const FWebBrowserInitSettings& WebBrowserInitSettings) override;
};

IMPLEMENT_MODULE(FChromiumUIModule, ChromiumUI)

void FChromiumUIModule::StartupModule()
{
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

bool FChromiumUIModule::CustomInitialize(const FWebBrowserInitSettings& WebBrowserInitSettings)
{
	if (WebBrowserSingleton == nullptr)
	{
		WebBrowserSingleton = new FWebBrowserSingleton(WebBrowserInitSettings);
		return true;
	}
	return false;
}

IWebBrowserSingleton* FChromiumUIModule::GetSingleton()
{
	if (WebBrowserSingleton == nullptr)
	{
		WebBrowserSingleton = new FWebBrowserSingleton(FWebBrowserInitSettings());
	}
	return WebBrowserSingleton;
}
