// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IChromiumWebBrowserSingleton;

/**
 * WebBrowser initialization settings, can be used to override default init behaviors.
 */
struct CHROMIUMUI_API FChromiumWebBrowserInitSettings
{
public:
	/**
	 * Default constructor. Initializes all members with default behavior values.
	 */
	FChromiumWebBrowserInitSettings();

	// The string which is appended to the browser's user-agent value.
	FString ProductVersion;
};

/**
 * WebBrowserModule interface
 */
class IChromiumWebBrowserModule : public IModuleInterface
{
public:
	/**
	 * Get or load the Web Browser Module
	 * 
	 * @return The loaded module
	 */
	static inline IChromiumWebBrowserModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IChromiumWebBrowserModule >("ChromiumUI");
	}
	
	/**
	 * Check whether the module has already been loaded
	 * 
	 * @return True if the module is loaded
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ChromiumUI");
	}

	/**
	 * Customize initialization settings. You must call this before the first GetSingleton call, in order to override init settings.
	 * 
	 * @param WebBrowserInitSettings The custom settings.
	 * @return true if the settings were used to initialize the singleton. False if the call was ignored due to singleton already existing.
	 */
	virtual bool CustomInitialize(const FChromiumWebBrowserInitSettings& WebBrowserInitSettings) = 0;

	/**
	 * Get the Web Browser Singleton
	 * 
	 * @return The Web Browser Singleton
	 */
	virtual IChromiumWebBrowserSingleton* GetSingleton() = 0;
};
